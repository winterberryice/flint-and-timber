#include "camera.hpp"

namespace flint
{
    namespace math
    {

        Camera::Camera()
        {
            // Default constructor - all member variables already initialized in header
            // Camera starts at (0, 0, 3) looking at origin (0, 0, 0)
        }

        void Camera::setPosition(const glm::vec3 &position)
        {
            m_position = position;
        }

        void Camera::setTarget(const glm::vec3 &target)
        {
            m_target = target;
        }

        void Camera::setPerspective(float fov, float aspect, float near, float far)
        {
            m_fov = fov;
            m_aspect = aspect;
            m_near = near;
            m_far = far;
        }

        glm::mat4 Camera::getViewMatrix() const
        {
            // Creates a "look at" matrix
            // Camera looks FROM m_position TO m_target with m_up as up direction
            return glm::lookAt(m_position, m_target, m_up);
        }

        glm::mat4 Camera::getProjectionMatrix() const
        {
            // Creates perspective projection matrix
            // fov in degrees, aspect ratio, near/far clipping planes
            return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
        }

        glm::mat4 Camera::getViewProjectionMatrix() const
        {
            // Combined transformation: Projection * View
            // Note: Matrix multiplication order is important!
            return getProjectionMatrix() * getViewMatrix();
        }

    } // namespace math
} // namespace flint
