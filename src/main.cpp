#include "FlintAndTimberApp.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Flint & Timber",
        800,
        600,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    try
    {
        FlintAndTimberApp app(window);

        bool is_running = true;
        SDL_Event event;

        while (is_running)
        {
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    is_running = false;
                }
            }

            app.Render();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
