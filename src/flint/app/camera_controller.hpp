#pragma once

#include "../math/camera.hpp"
#include <SDL3/SDL_events.h>

namespace flint
{
    namespace app
    {
        class CameraController
        {
        public:
            CameraController(float speed = 2.5f, float sensitivity = 0.1f);

            void handle_event(const SDL_Event &event);
            void update(class flint::math::Camera &camera, float delta_time);

        private:
            float m_speed;
            float m_sensitivity;

            float m_yaw = -90.0f;
            float m_pitch = 0.0f;

            bool m_w_pressed = false;
            bool m_a_pressed = false;
            bool m_s_pressed = false;
            bool m_d_pressed = false;
            bool m_f_pressed = false;
            bool m_space_pressed = false;
        };
    }
}
