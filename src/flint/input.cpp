#include "input.hpp"
#include <SDL3/SDL_mouse.h>

namespace flint
{
    void InputState::on_mouse_button(const SDL_MouseButtonEvent &event)
    {
        bool is_pressed = (event.state == SDL_PRESSED);
        if (event.button == SDL_BUTTON_LEFT)
        {
            if (is_pressed && !left_mouse_was_pressed_event)
            {
                left_mouse_pressed_this_frame = true;
            }
            if (!is_pressed && left_mouse_is_down)
            {
                left_mouse_released_this_frame = true;
            }
            left_mouse_is_down = is_pressed;
            left_mouse_was_pressed_event = is_pressed;
        }
        else if (event.button == SDL_BUTTON_RIGHT)
        {
            if (is_pressed && !right_mouse_was_pressed_event)
            {
                right_mouse_pressed_this_frame = true;
            }
            if (!is_pressed && right_mouse_is_down)
            {
                right_mouse_released_this_frame = true;
            }
            right_mouse_is_down = is_pressed;
            right_mouse_was_pressed_event = is_pressed;
        }
    }

    void InputState::on_mouse_motion(const SDL_MouseMotionEvent &event)
    {
        cursor_position.x = event.x;
        cursor_position.y = event.y;
    }

    void InputState::clear_frame_state()
    {
        left_mouse_pressed_this_frame = false;
        right_mouse_pressed_this_frame = false;
        left_mouse_released_this_frame = false;
        right_mouse_released_this_frame = false;
    }
} // namespace flint
