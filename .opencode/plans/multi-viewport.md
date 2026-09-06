# Multi-Viewport ImGui Implementation Plan

## Goal
Add multi-viewport support to the custom imgui platform backend for docking features. The imgui implementation should only use hg:: types, no SDL knowledge.

## Window API Additions (window.hpp + window.cpp)

### WindowConfig
Add `bool hidden = false` - viewport windows start hidden.

### Window methods
- `void show()` - shows a hidden window (`SDL_ShowWindow`)
- `f32 globalMouseX() const` - global mouse position (`SDL_GetGlobalMouseState`)
- `f32 globalMouseY() const` - global mouse position

### Window::create() change
Respect `config.hidden` - add `SDL_WINDOW_HIDDEN` flag when true.

## imgui.cpp Changes

### New struct
```cpp
struct ImGuiViewportData {
    Window* window = nullptr;
};
```

### ImGuiState additions
- `bool viewportsEnabled` flag

### initImGui() changes
- Set `ImGuiConfigFlags_ViewportsEnable`
- Set `ImGuiBackendFlags_PlatformHasViewports`
- Set `ImGuiBackendFlags_HasMouseHoveredViewport`
- Set `ImGuiBackendFlags_HasParentViewport`
- Register all platform callbacks on ImGuiPlatformIO
- Store main viewport's Window* in ImGuiViewportData, use as PlatformHandle

### Platform callbacks
All delegate to Window API:
- `Platform_CreateWindow` - `Window::create({.hidden=true})`, store in viewport data
- `Platform_DestroyWindow` - delete the Window
- `Platform_ShowWindow` - `window->show()`
- `Platform_SetWindowPos/GetWindowPos` - `window->setPosition()` / `window->posX()`/`posY()`
- `Platform_SetWindowSize/GetWindowSize` - `window->setSize()` / `window->width()`/`height()`
- `Platform_GetWindowFramebufferScale` - `window->scaleX()`/`scaleY()`
- `Platform_SetWindowFocus/GetWindowFocus` - `window->setFocus()` / `window->isFocused()`
- `Platform_GetWindowMinimized` - `window->isMinimized()`
- `Platform_SetWindowTitle` - `window->setTitle()`
- `Platform_SetWindowAlpha` - `window->setOpacity()`
- `Platform_CreateVkSurface` - NOT needed (handled by imgui_impl_vulkan's Renderer_CreateWindow)

### beginImGuiFrame() changes
1. Process events for ALL viewports:
   - Main viewport: `imguiState.window->events()` (existing)
   - Secondary viewports: iterate `pio.Viewports`, get `ImGuiViewportData*` from `PlatformUserData`, call `window->events()`
2. For secondary viewports, convert mouse coords to global space using `window->globalMouseX()`/`globalMouseY()`
3. Call `io.AddMouseViewportEvent()` for mouse hover tracking
4. Call `io.AddFocusEvent()` when focus changes per viewport

### deinitImGui() changes
- Destroy all secondary viewport windows before main window
- Clear viewport-related flags

## Files Modified
1. `include/hg/window.hpp` - WindowConfig::hidden, Window::show(), globalMouseX(), globalMouseY()
2. `src/sdl/window.cpp` - implementations
3. `src/imgui.cpp` - viewport data, platform callbacks, event routing

## Implementation Order
1. Window API additions (window.hpp + window.cpp)
2. ImGuiViewportData struct and platform callbacks (imgui.cpp)
3. initImGui() viewport setup (imgui.cpp)
4. beginImGuiFrame() multi-viewport event routing (imgui.cpp)
5. deinitImGui() cleanup (imgui.cpp)
6. Build and test
