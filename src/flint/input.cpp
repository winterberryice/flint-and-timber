#include "input.hpp"

namespace flint
{
    void InputState::on_event(const SDL_Event &event)
    {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            bool is_pressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (is_pressed && !left_mouse_is_down)
                {
                    left_mouse_pressed_this_frame = true;
                }
                if (!is_pressed && left_mouse_is_down)
                {
                    left_mouse_released_this_frame = true;
                }
                left_mouse_is_down = is_pressed;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                if (is_pressed && !right_mouse_is_down)
                {
                    right_mouse_pressed_this_frame = true;
                }
                if (!is_pressed && right_mouse_is_down)
                {
                    right_mouse_released_this_frame = true;
                }
                right_mouse_is_down = is_pressed;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            cursor_position.x = event.motion.x;
            cursor_position.y = event.motion.y;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            key_is_down[event.key.key] = true;
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            key_is_down[event.key.key] = false;
        }
    }

    void InputState::clear_frame_state()
    {
        left_mouse_pressed_this_frame = false;
        right_mouse_pressed_this_frame = false;
        left_mouse_released_this_frame = false;
        right_mouse_released_this_frame = false;
    }

    bool InputState::is_key_down(SDL_Keycode key) const
    {
        auto it = key_is_down.find(key);
        if (it != key_is_down.end())
        {
            return it->second;
        }
        return false;
    }
} // namespace flint
