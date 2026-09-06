# Plan: Replace SDL ImGui backend with HurdyGurdy backend

## Principle

The imgui backend knows only `hg::` types. SDL is an implementation detail hidden behind the HurdyGurdy windowing API. The Vulkan renderer backend (`imgui_impl_vulkan.cpp`) is unchanged.

## Files to modify

1. `src/imgui_hurdygurdy.h` (new) - Backend API header
2. `src/imgui.cpp` (modify) - Add backend implementation, keep existing public API
3. `CMakeLists.txt` (modify) - Remove `imgui_impl_sdl3.cpp` from build
4. `src/sdl/window.cpp` (modify) - Remove imgui_impl_sdl3 include/calls, expand event loop
5. `src/internal.hpp` (modify) - Remove window-side imgui declarations
6. `include/hg/window.hpp` (modify) - Expand WindowEvent, add state queries and stubs

## 1. Expand WindowEvent and Window state

### WindowEvent (`include/hg/window.hpp`)

Ordered events only. Position/delta/wheel/focus/size are state queries.

```cpp
enum WindowEventType : u32 {
    WindowEventType_none = 0,
    WindowEventType_buttonPress,
    WindowEventType_buttonRelease,
    WindowEventType_textInput,
    WindowEventType_count,
};

struct WindowEvent {
    WindowEventType type;
    Button button;          // buttonPress/buttonRelease
    char text[32];         // textInput (UTF-8, null-terminated)
};
```

### New Window state queries (`include/hg/window.hpp`)

```cpp
f32 wheelDX() const;  // accumulated horizontal wheel delta, cleared per processEvents()
f32 wheelDY() const;  // accumulated vertical wheel delta, cleared per processEvents()
```

Accumulated during `processEvents()`, cleared at the start of each call. Same pattern as `mouseDX`/`mouseDY`.

### Window multi-viewport stubs (`include/hg/window.hpp`)

All stubs (user implements later):

```cpp
// Platform handle (SDL_Window* wrapped as void*)
void* platformHandle() const;

// Multi-viewport window management
static Maybe<Window> createPlatformWindow(u32 width, u32 height, bool decoration);
void destroyPlatformWindow();
void setPlatformWindowPosition(i32 x, i32 y);
void setPlatformWindowSize(u32 w, u32 h);
void setPlatformWindowFocus();
bool isPlatformWindowFocused() const;
bool isPlatformWindowMinimized() const;
void setPlatformWindowTitle(StringView title);
void setPlatformWindowOpacity(f32 alpha);
void showPlatformWindow();

// Vulkan surface creation (for multi-viewport)
void* createPlatformSurface(void* instance, void* allocator) const;

// Display enumeration
struct DisplayInfo {
    i32 posX, posY;
    i32 sizeW, sizeH;
    i32 workPosX, workPosY;
    i32 workSizeW, workSizeH;
    f32 dpiScale;
};
static u32 displayCount();
static DisplayInfo displayInfo(u32 index);
```

## 2. Backend API (`src/imgui_hurdygurdy.h`)

```cpp
#pragma once

namespace hg {

struct Window;
struct WindowEvent;

bool hgImGuiInit(const Window& window);
void hgImGuiShutdown();
void hgImGuiNewFrame();
void hgImGuiProcessEvent(const WindowEvent& event);

} // namespace hg
```

## 3. Backend implementation (`src/imgui.cpp`)

Based on `imgui_impl_sdl3.cpp`, keeping only features the engine uses. SDL is included only in this translation unit.

### Backend data struct

```
struct ImGuiHurdyGurdyData {
    Window* window;           // main window
    u64 time;                 // performance counter
    i32 mouseButtonsDown;     // bitmask of held mouse buttons
    SDL_Cursor* cursors[ImGuiMouseCursor_COUNT];
    SDL_Cursor* lastCursor;
    bool wantUpdateMonitors;
    char* clipboardText;
};
```

### Key mapping

`sdlKeycodeToImGuiKey(u32 keycode, u32 scancode)` - maps SDL keycodes/scancodes to ImGuiKey values. Keypad handled via scancodes, main keys via keycodes, OEM keys via scancode fallback.

### `hgImGuiInit(const Window& window)`

- Allocate `ImGuiHurdyGurdyData`
- Set `io.BackendPlatformName`, `io.BackendPlatformUserData`
- Set flags: `HasMouseCursors`, `HasSetMousePos`, `PlatformHasViewports`, `HasParentViewport`
- Set platform I/O handlers: clipboard, IME, `OpenInShellFn`, full multi-viewport platform interface
- Load system mouse cursors
- Enumerate monitors via `Window::displayCount()`/`Window::displayInfo()`
- Set up main viewport: `PlatformHandle = window.platformHandle()`

### `hgImGuiShutdown()`

- `ImGui::DestroyPlatformWindows()`
- Free clipboard text
- Destroy cursors
- Clear backend flags
- `platform_io.ClearPlatformHandlers()`
- Delete backend data

### `hgImGuiNewFrame()`

- Query display size: `window.width()`, `window.height()`
- Query framebuffer scale: `window.platformFramebufferScale()` or default `1.0f`
- Calculate delta time from performance counter
- Update mouse position from `window.mouseX()`, `window.mouseY()`
- Update mouse delta from `window.mouseDX()`, `window.mouseDY()`
- Update mouse wheel from `window.wheelDX()`, `window.wheelDY()`
- Update cursor shape via `io.MouseDrawCursor` / `ImGui::GetMouseCursor()`
- Update monitors if needed

### `hgImGuiProcessEvent(const WindowEvent& event)`

- `buttonPress`/`buttonRelease`: `io.AddKeyEvent(ImGuiKey_*, down)`, `io.AddMouseButtonEvent(button, down)`
- `textInput`: `io.AddInputCharactersUTF8(event.text)`

### Multi-viewport platform interface

All functions delegate to `Window` API methods:

| ImGui callback | Window method |
|---|---|
| `Platform_CreateWindow` | `Window::createPlatformWindow()` |
| `Platform_DestroyWindow` | `window.destroyPlatformWindow()` |
| `Platform_ShowWindow` | `window.showPlatformWindow()` |
| `Platform_SetWindowPos` | `window.setPlatformWindowPosition()` |
| `Platform_GetWindowPos` | `window.platformWindowPosition()` |
| `Platform_SetWindowSize` | `window.setPlatformWindowSize()` |
| `Platform_GetWindowSize` | `window.platformWindowSize()` |
| `Platform_SetWindowFocus` | `window.setPlatformWindowFocus()` |
| `Platform_GetWindowFocus` | `window.isPlatformWindowFocused()` |
| `Platform_GetWindowMinimized` | `window.isPlatformWindowMinimized()` |
| `Platform_SetWindowTitle` | `window.setPlatformWindowTitle()` |
| `Platform_SetWindowAlpha` | `window.setPlatformWindowOpacity()` |
| `Platform_CreateVkSurface` | `window.createPlatformSurface()` |
| `Platform_RenderWindow` | no-op (Vulkan) |
| `Platform_SwapBuffers` | no-op (Vulkan) |

### Existing public API unchanged

`initImGui`, `deinitImGui`, `beginImGuiFrame`, `renderImGui`, `createImGuiTexture`, `destroyImGuiTexture` - all stay as-is.

## 4. CMakeLists.txt

Remove `imgui_impl_sdl3.cpp`:

```cmake
set(IMGUI_BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp
)
```

## 5. src/sdl/window.cpp

- Remove `#include <backends/imgui_impl_sdl3.h>`
- Remove `#include <imgui.h>`
- Remove the three `internal::initImGuiWindow`/`deinitImGuiWindow`/`beginImGuiFrameWindow` functions (lines 469-488)
- In `processEvents()`:
  - Add `wheelDX`/`wheelDY` accumulation for `SDL_EVENT_MOUSE_WHEEL`
  - For `SDL_EVENT_KEY_DOWN`/`SDL_EVENT_KEY_UP`, also emit text input for printable characters via `SDL_GetKeyboardState` / text event
  - Replace `ImGui_ImplSDL3_ProcessEvent(&event)` with per-event-type calls to `hg::hgImGuiProcessEvent(windowEvent)` for button and text events

## 6. src/internal.hpp

Remove:
```cpp
void initImGuiWindow(const Window& window);
void deinitImGuiWindow();
void beginImGuiFrameWindow();
```

Keep:
```cpp
void initImGuiGpu(...);
void deinitImGuiGpu();
void beginImGuiFrameGpu();
```

## Verification

1. `cmake --workflow --preset debug` - build succeeds
2. `./build/tests` - tests pass
3. `./build/minimal` - ImGui renders (stubs return defaults for multi-viewport)
