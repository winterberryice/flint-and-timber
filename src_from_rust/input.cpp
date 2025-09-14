#include "input.h"

namespace flint
{

    InputState::InputState() = default;

    void InputState::on_mouse_input(const SDL_MouseButtonEvent &event, bool inventory_open)
    {
        // inventory_open is unused for now, but kept for parity with Rust code
        (void)inventory_open;

        bool is_pressed = (event.down);

        switch (event.button)
        {
        case SDL_BUTTON_LEFT:
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
            break;

        case SDL_BUTTON_RIGHT:
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
            break;

        default:
            // Other buttons are ignored
            break;
        }
    }

    void InputState::on_cursor_moved(const SDL_MouseMotionEvent &event)
    {
        cursor_position = {event.x, event.y};
    }

    void InputState::clear_frame_state()
    {
        left_mouse_pressed_this_frame = false;
        right_mouse_pressed_this_frame = false;
        left_mouse_released_this_frame = false;
        right_mouse_released_this_frame = false;
    }

} // namespace flint
