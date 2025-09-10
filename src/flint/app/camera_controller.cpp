#include "camera_controller.hpp"
#include "../math/camera.hpp"
#include <iostream>

namespace flint
{
    namespace app
    {

        CameraController::CameraController(float speed, float sensitivity)
            : m_speed(speed), m_sensitivity(sensitivity)
        {
        }

        void CameraController::handle_event(const SDL_Event &event)
        {
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
            {
                bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.key)
                {
                case SDLK_W:
                    m_w_pressed = pressed;
                    break;
                case SDLK_A:
                    m_a_pressed = pressed;
                    break;
                case SDLK_S:
                    m_s_pressed = pressed;
                    break;
                case SDLK_D:
                    m_d_pressed = pressed;
                    break;
                case SDLK_SPACE:
                    m_space_pressed = pressed;
                    break;
                case SDLK_LSHIFT:
                    m_shift_pressed = pressed;
                    break;
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                m_yaw += event.motion.xrel * m_sensitivity;
                m_pitch -= event.motion.yrel * m_sensitivity;

                // Clamp pitch
                if (m_pitch > 89.0f)
                    m_pitch = 89.0f;
                if (m_pitch < -89.0f)
                    m_pitch = -89.0f;
            }
        }

        void CameraController::update(flint::math::Camera &camera, float delta_time)
        {
            float velocity = m_speed * delta_time;
            if (m_w_pressed)
                camera.move_forward(velocity);
            if (m_s_pressed)
                camera.move_forward(-velocity);
            if (m_a_pressed)
                camera.move_right(-velocity);
            if (m_d_pressed)
                camera.move_right(velocity);
            if (m_space_pressed)
                camera.move_up(velocity);
            if (m_shift_pressed)
                camera.move_up(-velocity);

            camera.update_vectors(m_yaw, m_pitch);
        }

    } // namespace app
} // namespace flint
