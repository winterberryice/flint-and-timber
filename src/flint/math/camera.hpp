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
            void setTarget(const glm::vec3 &target);
            void setPerspective(float fov, float aspect, float near, float far);

            // Get matrices
            glm::mat4 getViewMatrix() const;
            glm::mat4 getProjectionMatrix() const;
            glm::mat4 getViewProjectionMatrix() const;

            // Simple getters
            const glm::vec3 &getPosition() const { return m_position; }

        private:
            glm::vec3 m_position{0.0f, 0.0f, 3.0f}; // Camera position
            glm::vec3 m_target{0.0f, 0.0f, 0.0f};   // Looking at origin
            glm::vec3 m_up{0.0f, 1.0f, 0.0f};       // Up direction

            // Projection settings
            float m_fov{45.0f};
            float m_aspect{1.0f};
            float m_near{0.1f};
            float m_far{100.0f};
        };
    }
}