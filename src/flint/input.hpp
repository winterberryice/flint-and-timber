#pragma once

#include <SDL3/SDL_events.h>
#include <glm/glm.hpp>
#include <unordered_map>

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

        glm::vec2 cursor_position = {0.0f, 0.0f};
        std::unordered_map<SDL_Keycode, bool> key_is_down;

        void on_event(const SDL_Event &event);
        void clear_frame_state();
        bool is_key_down(SDL_Keycode key) const;
    };
} // namespace flint
