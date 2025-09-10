#include "camera.hpp"

namespace flint
{
    namespace math
    {

        Camera::Camera()
        {
            update_camera_vectors();
        }

        void Camera::setPosition(const glm::vec3 &position)
        {
            m_position = position;
        }

        void Camera::setPerspective(float fov, float aspect, float near, float far)
        {
            m_fov = fov;
            m_aspect = aspect;
            m_near = near;
            m_far = far;
        }

        void Camera::move_forward(float distance)
        {
            m_position += m_front * distance;
        }

        void Camera::move_right(float distance)
        {
            m_position += m_right * distance;
        }

        void Camera::move_up(float distance)
        {
            m_position += m_world_up * distance;
        }

        void Camera::update_vectors(float yaw, float pitch)
        {
            m_yaw = yaw;
            m_pitch = pitch;
            update_camera_vectors();
        }

        void Camera::update_camera_vectors()
        {
            glm::vec3 front;
            front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            front.y = sin(glm::radians(m_pitch));
            front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            m_front = glm::normalize(front);

            m_right = glm::normalize(glm::cross(m_front, m_world_up));
            m_up = glm::normalize(glm::cross(m_right, m_front));
        }

        glm::mat4 Camera::getViewMatrix() const
        {
            return glm::lookAt(m_position, m_position + m_front, m_up);
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
