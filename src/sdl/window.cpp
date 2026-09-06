#include "sdl_internal.hpp"

#include "internal.hpp"

#include "hg/window.hpp"
#include "hg/error.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include "vulkan/vulkan.h"

namespace hg {

struct WindowData {
    internal::Swapchain swap{};

    SDL_Window* sdlWindow = nullptr;
    f32 mouseX = 0;
    f32 mouseY = 0;
    bool isKeyDown[Button_count]{};
    bool wasClosed = false;
    Array<WindowEvent> events;

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept;
    WindowData& operator=(WindowData&& other) noexcept;

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

struct WindowState {
    Map<SDL_WindowID, WindowData*> ids{};

    f32 mouseDX = 0.0f;
    f32 mouseDY = 0.0f;
    f32 wheelDX = 0.0f;
    f32 wheelDY = 0.0f;
    bool wasQuit = false;

    SDL_Cursor* cursors[CursorType_count]{};
    SDL_Cursor* currentCursor = nullptr;

    DisplayInfo* displays = nullptr;
    u32 displayCount = 0;
};

static WindowState windowState{};

void windowInit()
{
    windowState = WindowState{};

    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    windowState.displayCount = static_cast<u32>(count);
    windowState.displays = new DisplayInfo[static_cast<u32>(count)];

    for (u32 n = 0; n < windowState.displayCount; n++)
    {
        DisplayInfo& info = windowState.displays[n];
        SDL_DisplayID displayId = ids[n];
        SDL_Rect r;
        SDL_GetDisplayBounds(displayId, &r);
        info.posX = r.x;
        info.posY = r.y;
        info.sizeW = r.w;
        info.sizeH = r.h;

        if (SDL_GetDisplayUsableBounds(displayId, &r) && r.w > 0 && r.h > 0)
        {
            info.workPosX = r.x;
            info.workPosY = r.y;
            info.workSizeW = r.w;
            info.workSizeH = r.h;
        }
        else
        {
            info.workPosX = info.posX;
            info.workPosY = info.posY;
            info.workSizeW = info.sizeW;
            info.workSizeH = info.sizeH;
        }

        info.dpiScale = SDL_GetDisplayContentScale(displayId);
    }
    SDL_free(ids);
}

void windowDeinit()
{
    for (u32 n = 0; n < CursorType_count; n++)
    {
        if (windowState.cursors[n] != nullptr)
            SDL_DestroyCursor(windowState.cursors[n]);
    }
    windowState.currentCursor = nullptr;

    delete[] windowState.displays;
    windowState.displays = nullptr;
    windowState.displayCount = 0;
}

WindowData::~WindowData() noexcept
{
    if (sdlWindow != nullptr)
    {
        windowState.ids.remove(SDL_GetWindowID(sdlWindow));
        SDL_DestroyWindow(sdlWindow);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::move(other.swap)}
    , sdlWindow{std::exchange(other.sdlWindow, nullptr)}
    , mouseX{std::exchange(other.mouseX, 0.0f)}
    , mouseY{std::exchange(other.mouseY, 0.0f)}
    , wasClosed{std::exchange(other.wasClosed, false)}
    , events{std::move(other.events)}
{
    memcpy(isKeyDown, other.isKeyDown, sizeof(isKeyDown));
    memset(other.isKeyDown, 0, sizeof(other.isKeyDown));
}

WindowData& WindowData::operator=(WindowData&& other) noexcept
{
    if (this != &other)
    {
        this->~WindowData();
        new (this) WindowData{std::move(other)};
    }
    return *this;
}

Window::Window() noexcept
    : data{nullptr}
{}

Window::~Window() noexcept = default;
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

Maybe<Window> Window::create(const WindowConfig& config)
{
    Maybe<Window> window = some<Window>();
    window->data = makeUnique<WindowData>();

    ArenaScope scratch = getScratch();

    u32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    if (config.hidden)
        flags |= SDL_WINDOW_HIDDEN;

    window->data->sdlWindow = SDL_CreateWindow(
        "Hurdy Gurdy",
        800, 600,
        flags);
    if (window->data->sdlWindow == nullptr)
    {
        setError(SDL_GetError());
        window->data = {};
        return {};
    }

    windowState.ids.add(SDL_GetWindowID(window->data->sdlWindow), window->data);

    u32 w, h;
    SDL_GetWindowSize(window->data->sdlWindow,
        reinterpret_cast<int*>(&w),
        reinterpret_cast<int*>(&h));

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(
        window->data->sdlWindow,
        static_cast<VkInstance>(internal::getVulkanInstance()),
        nullptr,
        &surface))
    {
        setError(SDL_GetError());
        goto surfaceFailed;
    }

    window->data->swap = internal::Swapchain::create(surface, w, h, config.preferredPresentMode, config.imageUsage);
    if (window->data->swap.data == nullptr)
    {
        windowState.ids.remove(SDL_GetWindowID(window->data->sdlWindow));
        goto windowFailed;
    }

    window->data->events = Array<WindowEvent>(0, 1024);

    return window;

windowFailed:
    vkDestroySurfaceKHR(static_cast<VkInstance>(internal::getVulkanInstance()), surface, nullptr);
surfaceFailed:
    SDL_DestroyWindow(window->data->sdlWindow);
    window->data = {};
    return {};
}

Format Window::imageFormat() const
{
    return data->swap.format();
}

GpuView* Window::imageView() const
{
    return data->swap.currentView();
}

void Window::setTitle(StringView title)
{
    ArenaScope scratch = getScratch();
    SDL_SetWindowTitle(data->sdlWindow, cString(scratch, title));
}

bool Window::wasClosed() const
{
    return data->wasClosed;
}

void Window::setFocus()
{
    SDL_RaiseWindow(data->sdlWindow);
}

bool Window::isFocused() const
{
    return SDL_GetMouseFocus() == data->sdlWindow;
}

bool Window::isMinimized() const
{
    return (SDL_GetWindowFlags(data->sdlWindow) & SDL_WINDOW_MINIMIZED) != 0;
}

void Window::setSize(u32 width, u32 height, bool resizeable)
{
    SDL_SetWindowSize(data->sdlWindow, static_cast<int>(width), static_cast<int>(height));
    data->swap.resize(width, height);
    SDL_SetWindowResizable(data->sdlWindow, resizeable);
}

void Window::setFullscreen(bool set)
{
    SDL_SetWindowFullscreen(data->sdlWindow, set ? SDL_WINDOW_FULLSCREEN : 0);

    u32 w, h;
    SDL_GetWindowSize(data->sdlWindow, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h));
    data->swap.resize(w, h);
}

u32 Window::width() const
{
    return data->swap.width();
}

u32 Window::height() const
{
    return data->swap.height();
}

f32 Window::scaleX() const
{
    int w, dw;
    SDL_GetWindowSize(data->sdlWindow, &w, nullptr);
    SDL_GetWindowSizeInPixels(data->sdlWindow, &dw, nullptr);
    return (w > 0) ? static_cast<f32>(dw) / static_cast<f32>(w) : 1.0f;
}

f32 Window::scaleY() const
{
    int h, dh;
    SDL_GetWindowSize(data->sdlWindow, nullptr, &h);
    SDL_GetWindowSizeInPixels(data->sdlWindow, nullptr, &dh);
    return (h > 0) ? static_cast<f32>(dh) / static_cast<f32>(h) : 1.0f;
}

void Window::setPosition(i32 x, i32 y)
{
    SDL_SetWindowPosition(data->sdlWindow, x, y);
}

u32 Window::posX() const
{
    i32 x;
    SDL_GetWindowPosition(data->sdlWindow, &x, nullptr);
    return static_cast<u32>(x);
}

u32 Window::posY() const
{
    i32 y;
    SDL_GetWindowPosition(data->sdlWindow, nullptr, &y);
    return static_cast<u32>(y);
}

void Window::setOpacity(f32 alpha)
{
    SDL_SetWindowOpacity(data->sdlWindow, alpha);
}

void Window::show()
{
    SDL_ShowWindow(data->sdlWindow);
}

f32 Window::globalMouseX() const
{
    f32 x;
    SDL_GetGlobalMouseState(&x, nullptr);
    return x;
}

f32 Window::globalMouseY() const
{
    f32 y;
    SDL_GetGlobalMouseState(nullptr, &y);
    return y;
}

f32 Window::mouseX() const
{
    return data->mouseX;
}

f32 Window::mouseY() const
{
    return data->mouseY;
}

f32 Window::mouseDX() const
{
    return windowState.mouseDX / static_cast<f32>(data->swap.height());
}

f32 Window::mouseDY() const
{
    return windowState.mouseDY / static_cast<f32>(data->swap.height());
}

f32 Window::wheelDX() const
{
    return windowState.wheelDX;
}

f32 Window::wheelDY() const
{
    return windowState.wheelDY;
}

bool Window::isButtonDown(Button key) const
{
    return data->isKeyDown[key];
}

Span<WindowEvent> Window::events() const
{
    return data->events;
}

static Button sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0: return Button_0;
        case SDLK_1: return Button_1;
        case SDLK_2: return Button_2;
        case SDLK_3: return Button_3;
        case SDLK_4: return Button_4;
        case SDLK_5: return Button_5;
        case SDLK_6: return Button_6;
        case SDLK_7: return Button_7;
        case SDLK_8: return Button_8;
        case SDLK_9: return Button_9;
        case SDLK_Q: return Button_q;
        case SDLK_W: return Button_w;
        case SDLK_E: return Button_e;
        case SDLK_R: return Button_r;
        case SDLK_T: return Button_t;
        case SDLK_Y: return Button_y;
        case SDLK_U: return Button_u;
        case SDLK_I: return Button_i;
        case SDLK_O: return Button_o;
        case SDLK_P: return Button_p;
        case SDLK_A: return Button_a;
        case SDLK_S: return Button_s;
        case SDLK_D: return Button_d;
        case SDLK_F: return Button_f;
        case SDLK_G: return Button_g;
        case SDLK_H: return Button_h;
        case SDLK_J: return Button_j;
        case SDLK_K: return Button_k;
        case SDLK_L: return Button_l;
        case SDLK_Z: return Button_z;
        case SDLK_X: return Button_x;
        case SDLK_C: return Button_c;
        case SDLK_V: return Button_v;
        case SDLK_B: return Button_b;
        case SDLK_N: return Button_n;
        case SDLK_M: return Button_m;
        case SDLK_SEMICOLON: return Button_semicolon;
        case SDLK_COLON: return Button_colon;
        case SDLK_APOSTROPHE: return Button_apostrophe;
        case SDLK_DBLAPOSTROPHE: return Button_quotation;
        case SDLK_COMMA: return Button_comma;
        case SDLK_PERIOD: return Button_period;
        case SDLK_QUESTION: return Button_question;
        case SDLK_GRAVE: return Button_grave;
        case SDLK_TILDE: return Button_tilde;
        case SDLK_EXCLAIM: return Button_exclamation;
        case SDLK_AT: return Button_at;
        case SDLK_HASH: return Button_hash;
        case SDLK_DOLLAR: return Button_dollar;
        case SDLK_PERCENT: return Button_percent;
        case SDLK_CARET: return Button_carot;
        case SDLK_AMPERSAND: return Button_ampersand;
        case SDLK_ASTERISK: return Button_asterisk;
        case SDLK_LEFTPAREN: return Button_lparen;
        case SDLK_RIGHTPAREN: return Button_rparen;
        case SDLK_LEFTBRACKET: return Button_lbracket;
        case SDLK_RIGHTBRACKET: return Button_rbracket;
        case SDLK_LEFTBRACE: return Button_lbrace;
        case SDLK_RIGHTBRACE: return Button_rbrace;
        case SDLK_EQUALS: return Button_equal;
        case SDLK_LESS: return Button_less;
        case SDLK_GREATER: return Button_greater;
        case SDLK_PLUS: return Button_plus;
        case SDLK_MINUS: return Button_minus;
        case SDLK_SLASH: return Button_slash;
        case SDLK_BACKSLASH: return Button_backslash;
        case SDLK_UNDERSCORE: return Button_underscore;
        case SDLK_PIPE: return Button_bar;
        case SDLK_UP: return Button_up;
        case SDLK_DOWN: return Button_down;
        case SDLK_LEFT: return Button_left;
        case SDLK_RIGHT: return Button_right;
        case SDLK_ESCAPE: return Button_escape;
        case SDLK_SPACE: return Button_space;
        case SDLK_RETURN: return Button_enter;
        case SDLK_BACKSPACE: return Button_backspace;
        case SDLK_DELETE: return Button_kdelete;
        case SDLK_INSERT: return Button_insert;
        case SDLK_TAB: return Button_tab;
        case SDLK_HOME: return Button_home;
        case SDLK_END: return Button_end;
        case SDLK_PAGEUP: return Button_pageup;
        case SDLK_PAGEDOWN: return Button_pagedown;
        case SDLK_F1: return Button_f1;
        case SDLK_F2: return Button_f2;
        case SDLK_F3: return Button_f3;
        case SDLK_F4: return Button_f4;
        case SDLK_F5: return Button_f5;
        case SDLK_F6: return Button_f6;
        case SDLK_F7: return Button_f7;
        case SDLK_F8: return Button_f8;
        case SDLK_F9: return Button_f9;
        case SDLK_F10: return Button_f10;
        case SDLK_F11: return Button_f11;
        case SDLK_F12: return Button_f12;
        case SDLK_LSHIFT: return Button_lshift;
        case SDLK_RSHIFT: return Button_rshift;
        case SDLK_LCTRL: return Button_lctrl;
        case SDLK_RCTRL: return Button_rctrl;
        case SDLK_LALT: return Button_lalt;
        case SDLK_RALT: return Button_ralt;
        case SDLK_LGUI: return Button_lsuper;
        case SDLK_RGUI: return Button_rsuper;
        case SDLK_CAPSLOCK: return Button_capslock;
        case SDLK_NUMLOCKCLEAR: return Button_numlock;
        case SDLK_SCROLLLOCK: return Button_scrolllock;
        case SDLK_PAUSE: return Button_pause;
    }
    return Button_none;
}

static Button sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT: return Button_mouse1;
        case SDL_BUTTON_RIGHT: return Button_mouse2;
        case SDL_BUTTON_MIDDLE: return Button_mouse3;
        case SDL_BUTTON_X1: return Button_mouse4;
        case SDL_BUTTON_X2: return Button_mouse5;
    }
    return Button_none;
}

void processEvents()
{
    windowState.mouseDX = 0;
    windowState.mouseDY = 0;
    windowState.wheelDX = 0;
    windowState.wheelDY = 0;

    windowState.ids.forEach([&](const SDL_WindowID&, WindowData*& window)
    {
        window->events.count = 0;
    });

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                windowState.wasQuit = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                WindowData** w = windowState.ids.get(event.window.windowID);
                if (w != nullptr)
                    (*w)->wasClosed = true;
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESTORED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowData** w = windowState.ids.get(event.window.windowID);
                if (w != nullptr)
                {
                    u32 w2, h2;
                    SDL_GetWindowSize((*w)->sdlWindow,
                        reinterpret_cast<int*>(&w2),
                        reinterpret_cast<int*>(&h2));
                    (*w)->swap.resize(w2, h2);
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    (*w)->mouseX = event.motion.x;
                    (*w)->mouseY = event.motion.y;
                }
                windowState.mouseDX += event.motion.xrel;
                windowState.mouseDY += event.motion.yrel;
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                windowState.wheelDX += event.wheel.x;
                windowState.wheelDY += event.wheel.y;
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.ids.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonPress;
                    windowEvent.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.ids.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonRelease;
                    windowEvent.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_TEXT_INPUT:
            {
                WindowData** w = windowState.ids.get(event.text.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_textInput;
                    memset(windowEvent.text, 0, sizeof(windowEvent.text));
                    strncpy(windowEvent.text, event.text.text, sizeof(windowEvent.text) - 1);

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonPress;
                    windowEvent.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonRelease;
                    windowEvent.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
        }
    }
}

bool wasQuit()
{
    return windowState.wasQuit;
}

Span<DisplayInfo> displayInfo()
{
    return Span<DisplayInfo>{windowState.displays, windowState.displayCount};
}

void setCursor(CursorType type)
{
    SDL_Cursor*& cursor = windowState.cursors[static_cast<u32>(type)];
    if (cursor == nullptr)
    {
        SDL_SystemCursor id;
        switch (type)
        {
            case CursorType_arrow:
                id = SDL_SYSTEM_CURSOR_DEFAULT;
                break;
            case CursorType_textInput:
                id = SDL_SYSTEM_CURSOR_TEXT;
                break;
            case CursorType_resizeAll:
                id = SDL_SYSTEM_CURSOR_MOVE;
                break;
            case CursorType_resizeNS:
                id = SDL_SYSTEM_CURSOR_NS_RESIZE;
                break;
            case CursorType_resizeEW:
                id = SDL_SYSTEM_CURSOR_EW_RESIZE;
                break;
            case CursorType_resizeNESW:
                id = SDL_SYSTEM_CURSOR_NESW_RESIZE;
                break;
            case CursorType_resizeNWSE:
                id = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
                break;
            case CursorType_hand:
                id = SDL_SYSTEM_CURSOR_POINTER;
                break;
            case CursorType_wait:
                id = SDL_SYSTEM_CURSOR_WAIT;
                break;
            case CursorType_progress:
                id = SDL_SYSTEM_CURSOR_PROGRESS;
                break;
            case CursorType_notAllowed:
                id = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
                break;
            default:
                id = SDL_SYSTEM_CURSOR_DEFAULT;
                break;
        }
        cursor = SDL_CreateSystemCursor(id);
    }

    if (windowState.currentCursor != cursor)
    {
        SDL_SetCursor(cursor);
        windowState.currentCursor = cursor;
    }
}

void showCursor()
{
    SDL_ShowCursor();
}

void hideCursor()
{
    SDL_HideCursor();
}

bool hasClipboardText()
{
    return SDL_HasClipboardText();
}

String getClipboardText()
{
    char* sdlText = SDL_GetClipboardText();
    String result = String::create(StringView{sdlText});
    SDL_free(sdlText);
    return result;
}

void setClipboardText(const char* text)
{
    SDL_SetClipboardText(text);
}

void openURL(const char* url)
{
    SDL_OpenURL(url);
}

} // namespace hg
