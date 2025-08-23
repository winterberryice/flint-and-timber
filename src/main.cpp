// Use angle brackets for SDL headers
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // Required for SDL_main entry point

#include <iostream>

int main(int argc, char *argv[])
{
    // Initialize the SDL library.
    // SDL_INIT_VIDEO is required for creating windows and rendering.
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window.
    // Parameters: title, width, height, flags.
    // SDL_WINDOW_RESIZABLE allows the user to resize the window.
    SDL_Window *window = SDL_CreateWindow(
        "My Basic SDL3 Window",
        800,
        600,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create a renderer to draw in the window.
    // In SDL3, the flags argument has been removed. SDL will pick the best
    // rendering driver by default when the second argument is nullptr.
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main application loop
    bool is_running = true;
    SDL_Event event;

    while (is_running)
    {
        // Process all pending events in the queue
        while (SDL_PollEvent(&event))
        {
            // Check the event type
            if (event.type == SDL_EVENT_QUIT)
            {
                // User requested to close the window (e.g., by clicking the 'X')
                is_running = false;
            }
        }

        // --- Drawing ---

        // Set the draw color to a nice blue
        SDL_SetRenderDrawColor(renderer, 40, 40, 90, 255);

        // Clear the entire screen with the current draw color
        SDL_RenderClear(renderer);

        //
        // You would add your drawing logic here
        //

        // Present the backbuffer to the screen to show the rendered content
        SDL_RenderPresent(renderer);
    }

    // --- Cleanup ---
    // Destroy the renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Shut down SDL subsystems
    SDL_Quit();

    std::cout << "SDL cleaned up successfully. Exiting." << std::endl;
    return 0;
}
