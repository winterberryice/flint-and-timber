#include "sdl.h"
#include "../debug.h"

#include <iostream>

SDL_Window *flint::init::sdl(uint32_t width, uint32_t height)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to initialize SDL");
    }

    printVideoSystemInfo();

    // Create window
    SDL_Window *window = SDL_CreateWindow("Flint & Timber", width, height, 0);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to create window");
    }

    printDetailedVideoInfo(window);

    std::cout << "SDL initialized successfully" << std::endl;

    return window;
}