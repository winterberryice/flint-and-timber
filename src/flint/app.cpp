#include "app.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>

#include "state.hpp"

App::App() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("Failed to initialize SDL");
    }

    window = SDL_CreateWindow("Voxel Game", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }

    // ... Dawn instance, adapter, and device creation ...

    // state = std::make_unique<State>(device, queue, surface, 1280, 720);
}

App::~App() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void App::set_mouse_grab(bool grab) {
    SDL_SetWindowMouseGrab(window, grab ? SDL_TRUE : SDL_FALSE);
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
    mouse_grabbed = grab;
}

void App::handle_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            // state->resize(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                if (mouse_grabbed) {
                    set_mouse_grab(false);
                } else {
                    running = false;
                }
            }
            // ... other key inputs ...
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            // state->input_state.on_mouse_button(event.button);
            if (!mouse_grabbed && event.button.button == SDL_BUTTON_LEFT && event.button.state == SDL_PRESSED) {
                set_mouse_grab(true);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (mouse_grabbed) {
                // state->player.process_mouse_movement(event.motion.xrel, event.motion.yrel);
            } else {
                // state->input_state.on_mouse_motion(event.motion);
            }
            break;
    }
}

void App::run() {
    set_mouse_grab(true);
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            handle_event(event);
        }

        // state->update();
        // state->render();

        // state->input_state.clear_frame_state();
    }
}
