#pragma once

#include <utility>
#include <cstdint>

#include <SDL3/SDL.h>

namespace flint
{
    struct InputState
    {
        bool left_mouse_pressed_this_frame = false;
        bool right_mouse_pressed_this_frame = false;
        bool left_mouse_released_this_frame = false;
        bool right_mouse_released_this_frame = false;
        bool left_mouse_is_down = false;
        bool right_mouse_is_down = false;

        std::pair<float, float> cursor_position = {0.0f, 0.0f};

        InputState();

        void on_mouse_input(const SDL_MouseButtonEvent &event, bool inventory_open);

        void on_cursor_moved(const SDL_MouseMotionEvent &event);

        void clear_frame_state();

    private:
        bool left_mouse_was_pressed_event = false;
        bool right_mouse_was_pressed_event = false;
    };
}
