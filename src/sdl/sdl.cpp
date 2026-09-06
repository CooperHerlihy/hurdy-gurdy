#include "internal.hpp"

#include "sdl/sdl_internal.hpp"
#include "hg/error.hpp"

namespace hg {
namespace internal {

bool initPlatform()
{
    if (!sdl::loadSDL())
        return false;

    if (!SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(static_cast<StringView>(SDL_GetError()));
        sdl::unloadSDL();
        return false;
    }
    return true;
}

void deinitPlatform()
{
    SDL_Quit();
    sdl::unloadSDL();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    Span<StringView> extBuffer{arena->alloc<StringView>(extCount), extCount};
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

} // namespace internal
} // namespace hg
