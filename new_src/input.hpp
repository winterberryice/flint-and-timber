#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>

struct InputState {
    bool left_mouse_pressed_this_frame = false;
    bool right_mouse_pressed_this_frame = false;
    bool left_mouse_released_this_frame = false;
    bool right_mouse_released_this_frame = false;
    bool left_mouse_is_down = false;
    bool right_mouse_is_down = false;
    glm::vec2 cursor_position = {0.0f, 0.0f};

    void on_mouse_button(const SDL_MouseButtonEvent& event);
    void on_mouse_motion(const SDL_MouseMotionEvent& event);
    void clear_frame_state();
private:
    bool left_mouse_was_pressed_event = false;
    bool right_mouse_was_pressed_event = false;
};
