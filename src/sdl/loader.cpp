#include "sdl/sdl_internal.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

namespace hg {
namespace sdl {

#define HG_MAKE_SDL_FUNC(name) decltype(&::name) name = nullptr

struct SdlFuncs {
    HG_MAKE_SDL_FUNC(SDL_Init);
    HG_MAKE_SDL_FUNC(SDL_Quit);
    HG_MAKE_SDL_FUNC(SDL_GetError);
    HG_MAKE_SDL_FUNC(SDL_CreateWindow);
    HG_MAKE_SDL_FUNC(SDL_DestroyWindow);
    HG_MAKE_SDL_FUNC(SDL_GetWindowID);
    HG_MAKE_SDL_FUNC(SDL_GetWindowSize);
    HG_MAKE_SDL_FUNC(SDL_GetWindowSizeInPixels);
    HG_MAKE_SDL_FUNC(SDL_GetWindowPosition);
    HG_MAKE_SDL_FUNC(SDL_SetWindowPosition);
    HG_MAKE_SDL_FUNC(SDL_SetWindowSize);
    HG_MAKE_SDL_FUNC(SDL_SetWindowTitle);
    HG_MAKE_SDL_FUNC(SDL_SetWindowOpacity);
    HG_MAKE_SDL_FUNC(SDL_SetWindowResizable);
    HG_MAKE_SDL_FUNC(SDL_SetWindowFullscreen);
    HG_MAKE_SDL_FUNC(SDL_ShowWindow);
    HG_MAKE_SDL_FUNC(SDL_RaiseWindow);
    HG_MAKE_SDL_FUNC(SDL_SetWindowParent);
    HG_MAKE_SDL_FUNC(SDL_GetWindowFlags);
    HG_MAKE_SDL_FUNC(SDL_GetWindowDisplayScale);
    HG_MAKE_SDL_FUNC(SDL_GetWindowRelativeMouseMode);
    HG_MAKE_SDL_FUNC(SDL_GetWindowFromID);
    HG_MAKE_SDL_FUNC(SDL_GetWindowProperties);
    HG_MAKE_SDL_FUNC(SDL_GetDisplays);
    HG_MAKE_SDL_FUNC(SDL_GetPrimaryDisplay);
    HG_MAKE_SDL_FUNC(SDL_GetFullscreenDisplayModes);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_MAKE_SDL_FUNC(SDL_WarpMouseInWindow);
    HG_MAKE_SDL_FUNC(SDL_WarpMouseGlobal);
    HG_MAKE_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_MAKE_SDL_FUNC(SDL_GetMouseFocus);
    HG_MAKE_SDL_FUNC(SDL_GetKeyboardFocus);
    HG_MAKE_SDL_FUNC(SDL_CaptureMouse);
    HG_MAKE_SDL_FUNC(SDL_GetKeyName);
    HG_MAKE_SDL_FUNC(SDL_GetScancodeName);
    HG_MAKE_SDL_FUNC(SDL_HasClipboardText);
    HG_MAKE_SDL_FUNC(SDL_GetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_SetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_CreateSystemCursor);
    HG_MAKE_SDL_FUNC(SDL_DestroyCursor);
    HG_MAKE_SDL_FUNC(SDL_SetCursor);
    HG_MAKE_SDL_FUNC(SDL_ShowCursor);
    HG_MAKE_SDL_FUNC(SDL_HideCursor);
    HG_MAKE_SDL_FUNC(SDL_SetHint);
    HG_MAKE_SDL_FUNC(SDL_PollEvent);
    HG_MAKE_SDL_FUNC(SDL_GetTicksNS);
    HG_MAKE_SDL_FUNC(SDL_GetPerformanceCounter);
    HG_MAKE_SDL_FUNC(SDL_GetPerformanceFrequency);
    HG_MAKE_SDL_FUNC(SDL_GetVersion);
    HG_MAKE_SDL_FUNC(SDL_GL_CreateContext);
    HG_MAKE_SDL_FUNC(SDL_GL_DestroyContext);
    HG_MAKE_SDL_FUNC(SDL_GL_GetCurrentContext);
    HG_MAKE_SDL_FUNC(SDL_GL_MakeCurrent);
    HG_MAKE_SDL_FUNC(SDL_GL_SetAttribute);
    HG_MAKE_SDL_FUNC(SDL_GL_SetSwapInterval);
    HG_MAKE_SDL_FUNC(SDL_GL_SwapWindow);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_MAKE_SDL_FUNC(SDL_OpenAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CloseAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CreateAudioStream);
    HG_MAKE_SDL_FUNC(SDL_DestroyAudioStream);
    HG_MAKE_SDL_FUNC(SDL_BindAudioStream);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamGetCallback);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_MAKE_SDL_FUNC(SDL_PutAudioStreamData);
    HG_MAKE_SDL_FUNC(SDL_GetAudioStreamQueued);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamGain);
    HG_MAKE_SDL_FUNC(SDL_StartTextInput);
    HG_MAKE_SDL_FUNC(SDL_StopTextInput);
    HG_MAKE_SDL_FUNC(SDL_TextInputActive);
    HG_MAKE_SDL_FUNC(SDL_SetTextInputArea);
    HG_MAKE_SDL_FUNC(SDL_OpenGamepad);
    HG_MAKE_SDL_FUNC(SDL_CloseGamepad);
    HG_MAKE_SDL_FUNC(SDL_GetGamepads);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadAxis);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadButton);
    HG_MAKE_SDL_FUNC(SDL_free);
    HG_MAKE_SDL_FUNC(SDL_OpenURL);
    HG_MAKE_SDL_FUNC(SDL_GetCurrentVideoDriver);
    HG_MAKE_SDL_FUNC(SDL_GetPointerProperty);
};

#undef HG_MAKE_SDL_FUNC

Library libsdl{};
SdlFuncs sdlFuncs{};

bool loadSDL()
{
    Maybe<Library> lib = Library::load(
#if defined(HG_PLATFORM_LINUX)
        "libSDL3.so.0"
#elif defined(HG_PLATFORM_WINDOWS)
        "SDL3.dll"
#elif defined(HG_PLATFORM_MACOS)
        "libSDL3.0.dylib"
#endif
    );
    if (!lib.has)
    {
        setError("Could not load SDL3");
        return false;
    }
    libsdl = std::move(*lib);

#define HG_LOAD_SDL_FUNC(name) \
    *(void**)&sdlFuncs.name = \
        libsdl.findFunction(#name).orElse(nullptr); \
    if (sdlFuncs.name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

    HG_LOAD_SDL_FUNC(SDL_Init);
    HG_LOAD_SDL_FUNC(SDL_Quit);
    HG_LOAD_SDL_FUNC(SDL_GetError);
    HG_LOAD_SDL_FUNC(SDL_CreateWindow);
    HG_LOAD_SDL_FUNC(SDL_DestroyWindow);
    HG_LOAD_SDL_FUNC(SDL_GetWindowID);
    HG_LOAD_SDL_FUNC(SDL_GetWindowSize);
    HG_LOAD_SDL_FUNC(SDL_GetWindowSizeInPixels);
    HG_LOAD_SDL_FUNC(SDL_GetWindowPosition);
    HG_LOAD_SDL_FUNC(SDL_SetWindowPosition);
    HG_LOAD_SDL_FUNC(SDL_SetWindowSize);
    HG_LOAD_SDL_FUNC(SDL_SetWindowTitle);
    HG_LOAD_SDL_FUNC(SDL_SetWindowOpacity);
    HG_LOAD_SDL_FUNC(SDL_SetWindowResizable);
    HG_LOAD_SDL_FUNC(SDL_SetWindowFullscreen);
    HG_LOAD_SDL_FUNC(SDL_ShowWindow);
    HG_LOAD_SDL_FUNC(SDL_RaiseWindow);
    HG_LOAD_SDL_FUNC(SDL_SetWindowParent);
    HG_LOAD_SDL_FUNC(SDL_GetWindowFlags);
    HG_LOAD_SDL_FUNC(SDL_GetWindowDisplayScale);
    HG_LOAD_SDL_FUNC(SDL_GetWindowRelativeMouseMode);
    HG_LOAD_SDL_FUNC(SDL_GetWindowFromID);
    HG_LOAD_SDL_FUNC(SDL_GetWindowProperties);
    HG_LOAD_SDL_FUNC(SDL_GetDisplays);
    HG_LOAD_SDL_FUNC(SDL_GetPrimaryDisplay);
    HG_LOAD_SDL_FUNC(SDL_GetFullscreenDisplayModes);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_LOAD_SDL_FUNC(SDL_WarpMouseInWindow);
    HG_LOAD_SDL_FUNC(SDL_WarpMouseGlobal);
    HG_LOAD_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_LOAD_SDL_FUNC(SDL_GetMouseFocus);
    HG_LOAD_SDL_FUNC(SDL_GetKeyboardFocus);
    HG_LOAD_SDL_FUNC(SDL_CaptureMouse);
    HG_LOAD_SDL_FUNC(SDL_GetKeyName);
    HG_LOAD_SDL_FUNC(SDL_GetScancodeName);
    HG_LOAD_SDL_FUNC(SDL_HasClipboardText);
    HG_LOAD_SDL_FUNC(SDL_GetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_SetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_CreateSystemCursor);
    HG_LOAD_SDL_FUNC(SDL_DestroyCursor);
    HG_LOAD_SDL_FUNC(SDL_SetCursor);
    HG_LOAD_SDL_FUNC(SDL_ShowCursor);
    HG_LOAD_SDL_FUNC(SDL_HideCursor);
    HG_LOAD_SDL_FUNC(SDL_SetHint);
    HG_LOAD_SDL_FUNC(SDL_PollEvent);
    HG_LOAD_SDL_FUNC(SDL_GetTicksNS);
    HG_LOAD_SDL_FUNC(SDL_GetPerformanceCounter);
    HG_LOAD_SDL_FUNC(SDL_GetPerformanceFrequency);
    HG_LOAD_SDL_FUNC(SDL_GetVersion);
    HG_LOAD_SDL_FUNC(SDL_GL_CreateContext);
    HG_LOAD_SDL_FUNC(SDL_GL_DestroyContext);
    HG_LOAD_SDL_FUNC(SDL_GL_GetCurrentContext);
    HG_LOAD_SDL_FUNC(SDL_GL_MakeCurrent);
    HG_LOAD_SDL_FUNC(SDL_GL_SetAttribute);
    HG_LOAD_SDL_FUNC(SDL_GL_SetSwapInterval);
    HG_LOAD_SDL_FUNC(SDL_GL_SwapWindow);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_LOAD_SDL_FUNC(SDL_OpenAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CloseAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CreateAudioStream);
    HG_LOAD_SDL_FUNC(SDL_DestroyAudioStream);
    HG_LOAD_SDL_FUNC(SDL_BindAudioStream);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamGetCallback);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_LOAD_SDL_FUNC(SDL_PutAudioStreamData);
    HG_LOAD_SDL_FUNC(SDL_GetAudioStreamQueued);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamGain);
    HG_LOAD_SDL_FUNC(SDL_StartTextInput);
    HG_LOAD_SDL_FUNC(SDL_StopTextInput);
    HG_LOAD_SDL_FUNC(SDL_TextInputActive);
    HG_LOAD_SDL_FUNC(SDL_SetTextInputArea);
    HG_LOAD_SDL_FUNC(SDL_OpenGamepad);
    HG_LOAD_SDL_FUNC(SDL_CloseGamepad);
    HG_LOAD_SDL_FUNC(SDL_GetGamepads);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadAxis);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadButton);
    HG_LOAD_SDL_FUNC(SDL_free);
    HG_LOAD_SDL_FUNC(SDL_OpenURL);
    HG_LOAD_SDL_FUNC(SDL_GetCurrentVideoDriver);
    HG_LOAD_SDL_FUNC(SDL_GetPointerProperty);

#undef HG_LOAD_SDL_FUNC

    return true;
}

void unloadSDL()
{
    libsdl = {};
}

} // namespace sdl
} // namespace hg

extern "C" bool SDLCALL SDL_Init(SDL_InitFlags flags)
{
    return ::hg::sdl::sdlFuncs.SDL_Init(flags);
}

extern "C" void SDLCALL SDL_Quit()
{
    ::hg::sdl::sdlFuncs.SDL_Quit();
}

extern "C" const char* SDLCALL SDL_GetError()
{
    return ::hg::sdl::sdlFuncs.SDL_GetError();
}

extern "C" SDL_Window* SDLCALL SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateWindow(title, w, h, flags);
}

extern "C" void SDLCALL SDL_DestroyWindow(SDL_Window* window)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyWindow(window);
}

extern "C" SDL_WindowID SDLCALL SDL_GetWindowID(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowID(window);
}

extern "C" bool SDLCALL SDL_GetWindowSize(SDL_Window* window, int* w, int* h)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowSize(window, w, h);
}

extern "C" bool SDLCALL SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* hp)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowSizeInPixels(window, w, hp);
}

extern "C" bool SDLCALL SDL_GetWindowPosition(SDL_Window* window, int* x, int* y)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowPosition(window, x, y);
}

extern "C" bool SDLCALL SDL_SetWindowPosition(SDL_Window* window, int x, int y)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowPosition(window, x, y);
}

extern "C" bool SDLCALL SDL_SetWindowSize(SDL_Window* window, int w, int h)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowSize(window, w, h);
}

extern "C" bool SDLCALL SDL_SetWindowTitle(SDL_Window* window, const char* title)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowTitle(window, title);
}

extern "C" bool SDLCALL SDL_SetWindowOpacity(SDL_Window* window, float opacity)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowOpacity(window, opacity);
}

extern "C" bool SDLCALL SDL_SetWindowResizable(SDL_Window* window, bool resizable)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowResizable(window, resizable);
}

extern "C" bool SDLCALL SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowFullscreen(window, fullscreen);
}

extern "C" bool SDLCALL SDL_ShowWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_ShowWindow(window);
}

extern "C" bool SDLCALL SDL_RaiseWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_RaiseWindow(window);
}

extern "C" bool SDLCALL SDL_SetWindowParent(SDL_Window* window, SDL_Window* parent)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowParent(window, parent);
}

extern "C" SDL_WindowFlags SDLCALL SDL_GetWindowFlags(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowFlags(window);
}

extern "C" float SDLCALL SDL_GetWindowDisplayScale(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowDisplayScale(window);
}

extern "C" bool SDLCALL SDL_GetWindowRelativeMouseMode(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowRelativeMouseMode(window);
}

extern "C" SDL_Window* SDLCALL SDL_GetWindowFromID(SDL_WindowID id)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowFromID(id);
}

extern "C" SDL_PropertiesID SDLCALL SDL_GetWindowProperties(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowProperties(window);
}

extern "C" SDL_DisplayID* SDLCALL SDL_GetDisplays(int* count)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplays(count);
}

extern "C" SDL_DisplayID SDLCALL SDL_GetPrimaryDisplay()
{
    return ::hg::sdl::sdlFuncs.SDL_GetPrimaryDisplay();
}

extern "C" SDL_DisplayMode** SDLCALL SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count)
{
    return ::hg::sdl::sdlFuncs.SDL_GetFullscreenDisplayModes(displayID, count);
}

extern "C" bool SDLCALL SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayBounds(displayID, rect);
}

extern "C" bool SDLCALL SDL_GetDisplayUsableBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayUsableBounds(displayID, rect);
}

extern "C" float SDLCALL SDL_GetDisplayContentScale(SDL_DisplayID displayID)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayContentScale(displayID);
}

extern "C" void SDLCALL SDL_WarpMouseInWindow(SDL_Window* window, float x, float y)
{
    ::hg::sdl::sdlFuncs.SDL_WarpMouseInWindow(window, x, y);
}

extern "C" bool SDLCALL SDL_WarpMouseGlobal(float x, float y)
{
    return ::hg::sdl::sdlFuncs.SDL_WarpMouseGlobal(x, y);
}

extern "C" SDL_MouseButtonFlags SDLCALL SDL_GetGlobalMouseState(float* x, float* y)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGlobalMouseState(x, y);
}

extern "C" SDL_Window* SDLCALL SDL_GetMouseFocus()
{
    return ::hg::sdl::sdlFuncs.SDL_GetMouseFocus();
}

extern "C" SDL_Window* SDLCALL SDL_GetKeyboardFocus()
{
    return ::hg::sdl::sdlFuncs.SDL_GetKeyboardFocus();
}

extern "C" bool SDLCALL SDL_CaptureMouse(bool capture)
{
    return ::hg::sdl::sdlFuncs.SDL_CaptureMouse(capture);
}

extern "C" const char* SDLCALL SDL_GetKeyName(SDL_Keycode key)
{
    return ::hg::sdl::sdlFuncs.SDL_GetKeyName(key);
}

extern "C" const char* SDLCALL SDL_GetScancodeName(SDL_Scancode scancode)
{
    return ::hg::sdl::sdlFuncs.SDL_GetScancodeName(scancode);
}

extern "C" bool SDLCALL SDL_HasClipboardText()
{
    return ::hg::sdl::sdlFuncs.SDL_HasClipboardText();
}

extern "C" char* SDLCALL SDL_GetClipboardText()
{
    return ::hg::sdl::sdlFuncs.SDL_GetClipboardText();
}

extern "C" bool SDLCALL SDL_SetClipboardText(const char* text)
{
    return ::hg::sdl::sdlFuncs.SDL_SetClipboardText(text);
}

extern "C" SDL_Cursor* SDLCALL SDL_CreateSystemCursor(SDL_SystemCursor id)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateSystemCursor(id);
}

extern "C" void SDLCALL SDL_DestroyCursor(SDL_Cursor* cursor)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyCursor(cursor);
}

extern "C" bool SDLCALL SDL_SetCursor(SDL_Cursor* cursor)
{
    return ::hg::sdl::sdlFuncs.SDL_SetCursor(cursor);
}

extern "C" bool SDLCALL SDL_ShowCursor()
{
    return ::hg::sdl::sdlFuncs.SDL_ShowCursor();
}

extern "C" bool SDLCALL SDL_HideCursor()
{
    return ::hg::sdl::sdlFuncs.SDL_HideCursor();
}

extern "C" bool SDLCALL SDL_SetHint(const char* name, const char* value)
{
    return ::hg::sdl::sdlFuncs.SDL_SetHint(name, value);
}

extern "C" bool SDLCALL SDL_PollEvent(SDL_Event* event)
{
    return ::hg::sdl::sdlFuncs.SDL_PollEvent(event);
}

extern "C" Uint64 SDLCALL SDL_GetTicksNS()
{
    return ::hg::sdl::sdlFuncs.SDL_GetTicksNS();
}

extern "C" Uint64 SDLCALL SDL_GetPerformanceCounter()
{
    return ::hg::sdl::sdlFuncs.SDL_GetPerformanceCounter();
}

extern "C" Uint64 SDLCALL SDL_GetPerformanceFrequency()
{
    return ::hg::sdl::sdlFuncs.SDL_GetPerformanceFrequency();
}

extern "C" int SDLCALL SDL_GetVersion()
{
    return ::hg::sdl::sdlFuncs.SDL_GetVersion();
}

extern "C" SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_CreateContext(window);
}

extern "C" bool SDLCALL SDL_GL_DestroyContext(SDL_GLContext context)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_DestroyContext(context);
}

extern "C" SDL_GLContext SDLCALL SDL_GL_GetCurrentContext()
{
    return ::hg::sdl::sdlFuncs.SDL_GL_GetCurrentContext();
}

extern "C" bool SDLCALL SDL_GL_MakeCurrent(SDL_Window* window, SDL_GLContext context)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_MakeCurrent(window, context);
}

extern "C" bool SDLCALL SDL_GL_SetAttribute(SDL_GLAttr attr, int value)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_SetAttribute(attr, value);
}

extern "C" bool SDLCALL SDL_GL_SetSwapInterval(int interval)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_SetSwapInterval(interval);
}

extern "C" bool SDLCALL SDL_GL_SwapWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GL_SwapWindow(window);
}

extern "C" bool SDLCALL SDL_Vulkan_CreateSurface(SDL_Window* window, VkInstance instance,
    const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
{
    return ::hg::sdl::sdlFuncs.SDL_Vulkan_CreateSurface(window, instance, allocator, surface);
}

extern "C" const char* const* SDLCALL SDL_Vulkan_GetInstanceExtensions(Uint32* count)
{
    return ::hg::sdl::sdlFuncs.SDL_Vulkan_GetInstanceExtensions(count);
}

extern "C" SDL_AudioDeviceID SDLCALL SDL_OpenAudioDevice(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenAudioDevice(devid, spec);
}

extern "C" void SDLCALL SDL_CloseAudioDevice(SDL_AudioDeviceID devid)
{
    ::hg::sdl::sdlFuncs.SDL_CloseAudioDevice(devid);
}

extern "C" SDL_AudioStream* SDLCALL SDL_CreateAudioStream(const SDL_AudioSpec* src_spec,
    const SDL_AudioSpec* dst_spec)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateAudioStream(src_spec, dst_spec);
}

extern "C" void SDLCALL SDL_DestroyAudioStream(SDL_AudioStream* stream)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyAudioStream(stream);
}

extern "C" bool SDLCALL SDL_BindAudioStream(SDL_AudioDeviceID devid, SDL_AudioStream* stream)
{
    return ::hg::sdl::sdlFuncs.SDL_BindAudioStream(devid, stream);
}

extern "C" bool SDLCALL SDL_SetAudioStreamGetCallback(SDL_AudioStream* stream, SDL_AudioStreamCallback callback, void *userdata)
{
    return ::hg::sdl::sdlFuncs.SDL_SetAudioStreamGetCallback(stream, callback, userdata);
}

extern "C" bool SDLCALL SDL_SetAudioStreamFormat(SDL_AudioStream* stream, const SDL_AudioSpec* src_spec,
    const SDL_AudioSpec* dst_spec)
{
    return ::hg::sdl::sdlFuncs.SDL_SetAudioStreamFormat(stream, src_spec, dst_spec);
}

extern "C" bool SDLCALL SDL_PutAudioStreamData(SDL_AudioStream* stream, const void* data, int len)
{
    return ::hg::sdl::sdlFuncs.SDL_PutAudioStreamData(stream, data, len);
}

extern "C" int SDLCALL SDL_GetAudioStreamQueued(SDL_AudioStream* stream)
{
    return ::hg::sdl::sdlFuncs.SDL_GetAudioStreamQueued(stream);
}

extern "C" bool SDLCALL SDL_SetAudioStreamGain(SDL_AudioStream* stream, float gain)
{
    return ::hg::sdl::sdlFuncs.SDL_SetAudioStreamGain(stream, gain);
}

extern "C" bool SDLCALL SDL_StartTextInput(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_StartTextInput(window);
}

extern "C" bool SDLCALL SDL_StopTextInput(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_StopTextInput(window);
}

extern "C" bool SDLCALL SDL_TextInputActive(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_TextInputActive(window);
}

extern "C" bool SDLCALL SDL_SetTextInputArea(SDL_Window* window, const SDL_Rect* rect, int cursor)
{
    return ::hg::sdl::sdlFuncs.SDL_SetTextInputArea(window, rect, cursor);
}

extern "C" SDL_Gamepad* SDLCALL SDL_OpenGamepad(SDL_JoystickID instance_id)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenGamepad(instance_id);
}

extern "C" void SDLCALL SDL_CloseGamepad(SDL_Gamepad* gamepad)
{
    ::hg::sdl::sdlFuncs.SDL_CloseGamepad(gamepad);
}

extern "C" SDL_JoystickID* SDLCALL SDL_GetGamepads(int* count)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepads(count);
}

extern "C" Sint16 SDLCALL SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepadAxis(gamepad, axis);
}

extern "C" bool SDLCALL SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepadButton(gamepad, button);
}

extern "C" void SDLCALL SDL_free(void* mem)
{
    ::hg::sdl::sdlFuncs.SDL_free(mem);
}

extern "C" bool SDLCALL SDL_OpenURL(const char* url)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenURL(url);
}

extern "C" const char* SDLCALL SDL_GetCurrentVideoDriver()
{
    return ::hg::sdl::sdlFuncs.SDL_GetCurrentVideoDriver();
}

extern "C" void* SDLCALL SDL_GetPointerProperty(SDL_PropertiesID props, const char* name, void* defaultVal)
{
    return ::hg::sdl::sdlFuncs.SDL_GetPointerProperty(props, name, defaultVal);
}
