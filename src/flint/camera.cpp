#include "camera.h"
#include <algorithm> // For std::clamp
#include <cmath>     // For sin and cos

namespace flint
{

    // --- Camera Implementation ---

    Camera::Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up, float aspect, float fovy_degrees, float znear, float zfar)
        : eye(eye),
          target(target),
          up(up),
          aspect(aspect),
          fovy_rads(glm::radians(fovy_degrees)), // Convert degrees to radians, like in Rust
          znear(znear),
          zfar(zfar)
    {
    }

    glm::mat4 Camera::buildViewProjectionMatrix() const
    {
        // glm::lookAt is equivalent to glam's Mat4::look_at_rh
        glm::mat4 view = glm::lookAt(eye, target, up);
        // glm::perspective is equivalent to glam's Mat4::perspective_rh
        glm::mat4 proj = glm::perspective(fovy_rads, aspect, znear, zfar);

        // GLM uses column-major matrices, so proj * view is correct.
        return proj * view;
    }

    // --- CameraUniform Implementation ---

    CameraUniform::CameraUniform()
    {
        // glm::mat4(1.0f) creates an identity matrix.
        view_proj = glm::mat4(1.0f);
    }

    void CameraUniform::updateViewProj(const Camera &camera, const glm::mat4 &model)
    {
        view_proj = camera.buildViewProjectionMatrix() * model;
    }

    // --- CameraMovement Implementation ---

    CameraMovement::CameraMovement(float speed, float mouse_sensitivity)
        : speed(speed), mouse_sensitivity(mouse_sensitivity) {}

    // --- CameraController Implementation ---

    CameraController::CameraController(float speed, float mouse_sensitivity)
        : movement(speed, mouse_sensitivity),
          m_yaw(glm::radians(-90.0f)), // Initialize to look along -Z
          m_pitch(0.0f)
    {
    }

    void CameraController::processMouseMovement(double delta_x, double delta_y)
    {
        float dx = static_cast<float>(delta_x) * movement.mouse_sensitivity;
        float dy = static_cast<float>(delta_y) * movement.mouse_sensitivity;

        m_yaw += dx;
        m_pitch -= dy; // Inverted

        // Clamp pitch to avoid flipping
        m_pitch = std::clamp(m_pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }

    void CameraController::updateCamera(Camera &camera, float dt_secs)
    {
        // Calculate forward and right vectors based on the current view direction
        glm::vec3 forward = glm::normalize(camera.target - camera.eye);
        glm::vec3 right = glm::normalize(glm::cross(forward, camera.up));

        if (movement.is_forward_pressed)
        {
            camera.eye += forward * movement.speed * dt_secs;
        }
        if (movement.is_backward_pressed)
        {
            camera.eye -= forward * movement.speed * dt_secs;
        }
        if (movement.is_left_pressed)
        {
            camera.eye -= right * movement.speed * dt_secs;
        }
        if (movement.is_right_pressed)
        {
            camera.eye += right * movement.speed * dt_secs;
        }
        // Use world-up for vertical movement to keep it simple
        if (movement.is_up_pressed)
        {
            camera.eye.y += movement.speed * dt_secs;
        }
        if (movement.is_down_pressed)
        {
            camera.eye.y -= movement.speed * dt_secs;
        }

        // Update the camera's target direction based on yaw and pitch
        glm::vec3 front;
        front.x = cos(m_yaw) * cos(m_pitch);
        front.y = sin(m_pitch);
        front.z = sin(m_yaw) * cos(m_pitch);

        camera.target = camera.eye + glm::normalize(front);
    }

} // namespace flint