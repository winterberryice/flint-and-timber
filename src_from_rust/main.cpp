#include "app.h"
#include <SDL3/SDL.h>
#include <iostream>

// Forward declaration for the function in app.cpp that gets the surface
// TODO: This is not a clean way to do this. A better design would be to pass the window to the App constructor.
// I have updated the App constructor to take the window, so this is not needed.

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // TODO: The window flags should be chosen carefully.
    // For Dawn, we might need specific flags if not using the GetWGPUSurface helper.
    SDL_Window* window = SDL_CreateWindow(
        "Flint (C++ Port)",
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    try {
        flint::App app(window);

        bool running = true;
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                app.handle_input(event); // Let the app handle event logic
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                 if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        // TODO: This is a simplified exit condition.
                        // The original Rust code has more complex logic for Esc key.
                        running = false;
                    }
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    app.process_mouse_motion(event.motion.xrel, event.motion.yrel);
                }
            }

            app.update();
            app.render();
        }

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }


    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
