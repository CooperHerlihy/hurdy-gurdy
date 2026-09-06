#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_audio.h>

namespace hg {

void windowInit();
void windowDeinit();

namespace sdl {

bool loadSDL();
void unloadSDL();

} // namespace sdl
} // namespace hg
