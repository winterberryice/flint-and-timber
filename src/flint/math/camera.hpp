#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace flint
{
    namespace math
    {

        class Camera
        {
        public:
            Camera();
            ~Camera() = default;

            // Basic camera setup
            void setPosition(const glm::vec3 &position);
            void setPerspective(float fov, float aspect, float near, float far);

            // Movement
            void move_forward(float distance);
            void move_right(float distance);
            void move_up(float distance);

            // Orientation
            void update_vectors(float yaw, float pitch);

            // Get matrices
            glm::mat4 getViewMatrix() const;
            glm::mat4 getProjectionMatrix() const;
            glm::mat4 getViewProjectionMatrix() const;

            // Simple getters
            const glm::vec3 &getPosition() const { return m_position; }

        private:
            void update_camera_vectors();

            glm::vec3 m_position{0.0f, 0.0f, 3.0f}; // Camera position
            glm::vec3 m_front{0.0f, 0.0f, -1.0f};
            glm::vec3 m_up;    // Calculated
            glm::vec3 m_right; // Calculated
            glm::vec3 m_world_up{0.0f, 1.0f, 0.0f};

            // Euler Angles
            float m_yaw = -90.0f;
            float m_pitch = 0.0f;

            // Projection settings
            float m_fov{45.0f};
            float m_aspect{1.0f};
            float m_near{0.1f};
            float m_far{100.0f};
        };
    }
}