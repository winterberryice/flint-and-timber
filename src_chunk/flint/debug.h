#pragma once

#include <SDL3/SDL.h>

namespace flint
{
    void printVideoSystemInfo();
    void printDetailedVideoInfo(SDL_Window *window);
}