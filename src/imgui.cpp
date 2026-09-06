#include "hg/imgui.hpp"

#include "internal.hpp"

#include "hg/window.hpp"
#include "hg/time.hpp"
#include "hg/array.hpp"

#include <imgui.h>

namespace hg {

struct ImGuiState {
    Window* window = nullptr;
    Clock clock{};

    CursorType cursors[ImGuiMouseCursor_COUNT]{};
    CursorType lastCursor = CursorType_count;

    String clipboardText;

    Array<UniquePtr<Window>> ownedWindows;
};

static ImGuiState state;

static ImGuiKey hgButtonToImGuiKey(Button button)
{
    switch (button)
    {
        case Button_0: return ImGuiKey_0;
        case Button_1: return ImGuiKey_1;
        case Button_2: return ImGuiKey_2;
        case Button_3: return ImGuiKey_3;
        case Button_4: return ImGuiKey_4;
        case Button_5: return ImGuiKey_5;
        case Button_6: return ImGuiKey_6;
        case Button_7: return ImGuiKey_7;
        case Button_8: return ImGuiKey_8;
        case Button_9: return ImGuiKey_9;
        case Button_a: return ImGuiKey_A;
        case Button_b: return ImGuiKey_B;
        case Button_c: return ImGuiKey_C;
        case Button_d: return ImGuiKey_D;
        case Button_e: return ImGuiKey_E;
        case Button_f: return ImGuiKey_F;
        case Button_g: return ImGuiKey_G;
        case Button_h: return ImGuiKey_H;
        case Button_i: return ImGuiKey_I;
        case Button_j: return ImGuiKey_J;
        case Button_k: return ImGuiKey_K;
        case Button_l: return ImGuiKey_L;
        case Button_m: return ImGuiKey_M;
        case Button_n: return ImGuiKey_N;
        case Button_o: return ImGuiKey_O;
        case Button_p: return ImGuiKey_P;
        case Button_q: return ImGuiKey_Q;
        case Button_r: return ImGuiKey_R;
        case Button_s: return ImGuiKey_S;
        case Button_t: return ImGuiKey_T;
        case Button_u: return ImGuiKey_U;
        case Button_v: return ImGuiKey_V;
        case Button_w: return ImGuiKey_W;
        case Button_x: return ImGuiKey_X;
        case Button_y: return ImGuiKey_Y;
        case Button_z: return ImGuiKey_Z;
        case Button_f1: return ImGuiKey_F1;
        case Button_f2: return ImGuiKey_F2;
        case Button_f3: return ImGuiKey_F3;
        case Button_f4: return ImGuiKey_F4;
        case Button_f5: return ImGuiKey_F5;
        case Button_f6: return ImGuiKey_F6;
        case Button_f7: return ImGuiKey_F7;
        case Button_f8: return ImGuiKey_F8;
        case Button_f9: return ImGuiKey_F9;
        case Button_f10: return ImGuiKey_F10;
        case Button_f11: return ImGuiKey_F11;
        case Button_f12: return ImGuiKey_F12;
        case Button_escape: return ImGuiKey_Escape;
        case Button_space: return ImGuiKey_Space;
        case Button_enter: return ImGuiKey_Enter;
        case Button_backspace: return ImGuiKey_Backspace;
        case Button_kdelete: return ImGuiKey_Delete;
        case Button_insert: return ImGuiKey_Insert;
        case Button_tab: return ImGuiKey_Tab;
        case Button_home: return ImGuiKey_Home;
        case Button_end: return ImGuiKey_End;
        case Button_pageup: return ImGuiKey_PageUp;
        case Button_pagedown: return ImGuiKey_PageDown;
        case Button_up: return ImGuiKey_UpArrow;
        case Button_down: return ImGuiKey_DownArrow;
        case Button_left: return ImGuiKey_LeftArrow;
        case Button_right: return ImGuiKey_RightArrow;
        case Button_lshift: return ImGuiKey_LeftShift;
        case Button_rshift: return ImGuiKey_RightShift;
        case Button_lctrl: return ImGuiKey_LeftCtrl;
        case Button_rctrl: return ImGuiKey_RightCtrl;
        case Button_lalt: return ImGuiKey_LeftAlt;
        case Button_ralt: return ImGuiKey_RightAlt;
        case Button_lsuper: return ImGuiKey_LeftSuper;
        case Button_rsuper: return ImGuiKey_RightSuper;
        case Button_capslock: return ImGuiKey_CapsLock;
        case Button_numlock: return ImGuiKey_NumLock;
        case Button_scrolllock: return ImGuiKey_ScrollLock;
        case Button_pause: return ImGuiKey_Pause;
        case Button_semicolon: return ImGuiKey_Semicolon;
        case Button_comma: return ImGuiKey_Comma;
        case Button_period: return ImGuiKey_Period;
        case Button_slash: return ImGuiKey_Slash;
        case Button_backslash: return ImGuiKey_Backslash;
        case Button_equal: return ImGuiKey_Equal;
        case Button_grave: return ImGuiKey_GraveAccent;
        case Button_apostrophe: return ImGuiKey_Apostrophe;
        case Button_lbracket: return ImGuiKey_LeftBracket;
        case Button_rbracket: return ImGuiKey_RightBracket;
        case Button_minus: return ImGuiKey_Minus;
        default: return ImGuiKey_None;
    }
}

static void processWindowEvents(ImGuiIO& io, Window* window)
{
    Span<WindowEvent> events = window->events();
    for (u32 n = 0; n < events.count; n++)
    {
        const WindowEvent& event = events[n];

        switch (event.type)
        {
            case WindowEventType_buttonPress:
            case WindowEventType_buttonRelease:
            {
                bool down = (event.type == WindowEventType_buttonPress);
                int mouseButton = -1;

                switch (event.button)
                {
                    case Button_mouse1: mouseButton = 0; break;
                    case Button_mouse2: mouseButton = 1; break;
                    case Button_mouse3: mouseButton = 2; break;
                    case Button_mouse4: mouseButton = 3; break;
                    case Button_mouse5: mouseButton = 4; break;
                    default: break;
                }

                if (mouseButton != -1)
                {
                    io.AddMouseButtonEvent(mouseButton, down);
                }
                else
                {
                    ImGuiKey key = hgButtonToImGuiKey(event.button);
                    if (key != ImGuiKey_None)
                        io.AddKeyEvent(key, down);

                    switch (event.button)
                    {
                        case Button_lctrl:
                        case Button_rctrl:
                            io.AddKeyEvent(ImGuiMod_Ctrl, down); break;
                        case Button_lshift:
                        case Button_rshift:
                            io.AddKeyEvent(ImGuiMod_Shift, down); break;
                        case Button_lalt:
                        case Button_ralt:
                            io.AddKeyEvent(ImGuiMod_Alt, down); break;
                        case Button_lsuper:
                        case Button_rsuper:
                            io.AddKeyEvent(ImGuiMod_Super, down); break;
                        default: break;
                    }
                }
            } break;

            case WindowEventType_textInput:
            {
                io.AddInputCharactersUTF8(event.text);
            } break;

            default:
                break;
        }
    }
}

static void platformCreateWindow(ImGuiViewport* viewport)
{
    WindowConfig config;
    config.hidden = true;

    Maybe<Window> window = Window::create(config);
    if (window.has)
    {
        UniquePtr<Window> owned = makeUnique<Window>(std::move(window.val));
        viewport->PlatformUserData = owned;
        viewport->PlatformHandle = owned;
        state.ownedWindows.push(std::move(owned));
    }
}

static void platformDestroyWindow(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
    {
        for (u64 i = 0; i < state.ownedWindows.count; i++)
        {
            if (state.ownedWindows[i] == window)
            {
                state.ownedWindows.removeSwap(i);
                break;
            }
        }
    }
    viewport->PlatformUserData = nullptr;
    viewport->PlatformHandle = nullptr;
}

static void platformShowWindow(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->show();
}

static ImVec2 platformGetWindowPos(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        return ImVec2(static_cast<f32>(window->posX()), static_cast<f32>(window->posY()));
    return ImVec2(0, 0);
}

static void platformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->setPosition(static_cast<i32>(pos.x), static_cast<i32>(pos.y));
}

static ImVec2 platformGetWindowSize(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        return ImVec2(static_cast<f32>(window->width()), static_cast<f32>(window->height()));
    return ImVec2(0, 0);
}

static void platformSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->setSize(static_cast<u32>(size.x), static_cast<u32>(size.y));
}

static ImVec2 platformGetWindowFramebufferScale(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        return ImVec2(window->scaleX(), window->scaleY());
    return ImVec2(1, 1);
}

static void platformSetWindowFocus(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->setFocus();
}

static bool platformGetWindowFocus(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        return window->isFocused();
    return false;
}

static bool platformGetWindowMinimized(ImGuiViewport* viewport)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        return window->isMinimized();
    return false;
}

static void platformSetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->setTitle(title);
}

static void platformSetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    Window* window = static_cast<Window*>(viewport->PlatformUserData);
    if (window != nullptr)
        window->setOpacity(alpha);
}

static int platformCreateVkSurface(ImGuiViewport* viewport, ImU64 vkInstance, const void* vkAllocator, ImU64* outVkSurface)
{
    (void)viewport;
    (void)vkInstance;
    (void)vkAllocator;
    (void)outVkSurface;
    return 1;
}

void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();

    state.window = const_cast<Window*>(&window);
    state.clock = Clock{};
    state.lastCursor = CursorType_count;

    io.BackendPlatformName = "imgui_hurdygurdy";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    state.cursors[ImGuiMouseCursor_Arrow] = CursorType_arrow;
    state.cursors[ImGuiMouseCursor_TextInput] = CursorType_textInput;
    state.cursors[ImGuiMouseCursor_ResizeAll] = CursorType_resizeAll;
    state.cursors[ImGuiMouseCursor_ResizeNS] = CursorType_resizeNS;
    state.cursors[ImGuiMouseCursor_ResizeEW] = CursorType_resizeEW;
    state.cursors[ImGuiMouseCursor_ResizeNESW] = CursorType_resizeNESW;
    state.cursors[ImGuiMouseCursor_ResizeNWSE] = CursorType_resizeNWSE;
    state.cursors[ImGuiMouseCursor_Hand] = CursorType_hand;
    state.cursors[ImGuiMouseCursor_Wait] = CursorType_wait;
    state.cursors[ImGuiMouseCursor_Progress] = CursorType_progress;
    state.cursors[ImGuiMouseCursor_NotAllowed] = CursorType_notAllowed;

    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) -> void
    {
        setClipboardText(text);
    };
    platformIO.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char*
    {
        if (state.clipboardText.length == 0 && hasClipboardText())
            state.clipboardText = getClipboardText();
        return state.clipboardText.chars;
    };
    platformIO.Platform_OpenInShellFn = [](ImGuiContext*, const char* url) -> bool { openURL(url); return true; };

    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    pio.Monitors.resize(0);

    Span<DisplayInfo> displays = displayInfo();
    for (u32 n = 0; n < displays.count; n++)
    {
        DisplayInfo& info = displays[n];
        ImGuiPlatformMonitor monitor;
        monitor.MainPos = monitor.WorkPos = ImVec2(
            static_cast<f32>(info.posX),
            static_cast<f32>(info.posY));
        monitor.MainSize = monitor.WorkSize = ImVec2(
            static_cast<f32>(info.sizeW),
            static_cast<f32>(info.sizeH));
        if (info.workSizeW > 0 && info.workSizeH > 0)
        {
            monitor.WorkPos = ImVec2(
                static_cast<f32>(info.workPosX),
                static_cast<f32>(info.workPosY));
            monitor.WorkSize = ImVec2(
                static_cast<f32>(info.workSizeW),
                static_cast<f32>(info.workSizeH));
        }
        monitor.DpiScale = info.dpiScale;
        monitor.PlatformHandle = reinterpret_cast<void*>(static_cast<intptr_t>(n));
        if (monitor.DpiScale <= 0.0f)
            continue;
        pio.Monitors.push_back(monitor);
    }

    internal::initImGuiGpu(
        *reinterpret_cast<const internal::Swapchain*>(window.data.ptr),
        colorFormat,
        depthFormat,
        stencilFormat);

    // Multi-viewport support
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    io.BackendFlags |= ImGuiBackendFlags_HasParentViewport;

    platformIO.Platform_CreateWindow = platformCreateWindow;
    platformIO.Platform_DestroyWindow = platformDestroyWindow;
    platformIO.Platform_ShowWindow = platformShowWindow;
    platformIO.Platform_UpdateWindow = [](ImGuiViewport*) {};
    platformIO.Platform_SetWindowPos = platformSetWindowPos;
    platformIO.Platform_GetWindowPos = platformGetWindowPos;
    platformIO.Platform_SetWindowSize = platformSetWindowSize;
    platformIO.Platform_GetWindowSize = platformGetWindowSize;
    platformIO.Platform_GetWindowFramebufferScale = platformGetWindowFramebufferScale;
    platformIO.Platform_SetWindowFocus = platformSetWindowFocus;
    platformIO.Platform_GetWindowFocus = platformGetWindowFocus;
    platformIO.Platform_GetWindowMinimized = platformGetWindowMinimized;
    platformIO.Platform_SetWindowTitle = platformSetWindowTitle;
    platformIO.Platform_RenderWindow = [](ImGuiViewport*, void*) {};
    platformIO.Platform_SwapBuffers = [](ImGuiViewport*, void*) {};
    platformIO.Platform_SetWindowAlpha = platformSetWindowAlpha;
    platformIO.Platform_CreateVkSurface = platformCreateVkSurface;

    // Register main viewport
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformUserData = state.window;
    mainViewport->PlatformHandle = state.window;
}

void deinitImGui()
{
    internal::deinitImGuiGpu();

    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    ImGui::DestroyPlatformWindows();

    state.clipboardText = String{};

    // Clean up main viewport (but not the window - it's owned by the caller)
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformUserData = nullptr;
    mainViewport->PlatformHandle = nullptr;

    io.BackendPlatformName = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors
        | ImGuiBackendFlags_HasSetMousePos
        | ImGuiBackendFlags_HasGamepad
        | ImGuiBackendFlags_PlatformHasViewports
        | ImGuiBackendFlags_HasParentViewport);
    platformIO.ClearPlatformHandlers();
}

void beginImGuiFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Process events for all viewports
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    for (int n = 0; n < pio.Viewports.Size; n++)
    {
        ImGuiViewport* viewport = pio.Viewports[n];
        Window* window = static_cast<Window*>(viewport->PlatformUserData);
        if (window == nullptr)
            continue;

        bool isMainViewport = (window == state.window);
        processWindowEvents(io, window);

        // Update mouse position
        if (isMainViewport)
        {
            io.AddMousePosEvent(window->mouseX(), window->mouseY());
        }
        else
        {
            // Secondary viewports use global mouse coordinates
            io.AddMousePosEvent(window->globalMouseX(), window->globalMouseY());
        }
    }

    // Update display size for main viewport
    io.DisplaySize = ImVec2(
        static_cast<f32>(state.window->width()),
        static_cast<f32>(state.window->height()));
    io.DisplayFramebufferScale = ImVec2(state.window->scaleX(), state.window->scaleY());

    io.DeltaTime = static_cast<f32>(state.clock.tick());
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 1.0f / 60.0f;

    // Mouse wheel (from main window)
    f32 wheelDX = state.window->wheelDX();
    f32 wheelDY = state.window->wheelDY();
    if (wheelDX != 0.0f || wheelDY != 0.0f)
        io.AddMouseWheelEvent(wheelDX, wheelDY);

    // Mouse cursor
    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None)
        {
            hideCursor();
        }
        else
        {
            CursorType expected = state.cursors[imguiCursor];
            if (state.lastCursor != expected)
            {
                setCursor(expected);
                state.lastCursor = expected;
            }
            showCursor();
        }
    }

    internal::beginImGuiFrameGpu();
}

} // namespace hg
