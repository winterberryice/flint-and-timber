#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace flint
{

    // Equivalent to the Rust `pub struct Camera`.
    struct Camera
    {
        glm::vec3 eye;
        glm::vec3 target;
        glm::vec3 up;
        float aspect;
        float fovy_rads; // Field of view in Y, in radians
        float znear;
        float zfar;

        // Constructor, equivalent to `Camera::new`.
        Camera(
            glm::vec3 eye = {0.0f, 0.0f, 3.0f},
            glm::vec3 target = {0.0f, 0.0f, 0.0f},
            glm::vec3 up = {0.0f, 1.0f, 0.0f},
            float aspect = 16.0f / 9.0f,
            float fovy_degrees = 45.0f,
            float znear = 0.1f,
            float zfar = 100.0f);

        // Const member function, equivalent to `build_view_projection_matrix`.
        glm::mat4 buildViewProjectionMatrix() const;
    };

    // Equivalent to `CameraUniform`. This struct is designed to be sent to the GPU.
    // glm::mat4 already has a memory layout compatible with GPU standards.
    struct CameraUniform
    {
    // We can use alignas to be explicit about memory alignment if needed,
    // but for a single mat4, it's usually fine.
        alignas(16) glm::mat4 view_proj;

        CameraUniform();

        void updateViewProj(const Camera &camera);
    };

// Uniform struct for model matrix
struct ModelUniform
{
    alignas(16) glm::mat4 model;
};

    // Equivalent to `CameraMovement`.
    struct CameraMovement
    {
        bool is_forward_pressed = false;
        bool is_backward_pressed = false;
        bool is_left_pressed = false;
        bool is_right_pressed = false;
        bool is_up_pressed = false;
        bool is_down_pressed = false;
        float mouse_sensitivity;
        float speed;

        CameraMovement(float speed = 5.0f, float mouse_sensitivity = 0.1f);
    };

    // Equivalent to `CameraController`.
    // A class is used here to encapsulate the private yaw/pitch state.
    class CameraController
    {
    public:
        CameraMovement movement;

        CameraController(float speed = 5.0f, float mouse_sensitivity = 0.1f);

        void processMouseMovement(double delta_x, double delta_y);
        void updateCamera(Camera &camera, float dt_secs);

    private:
        float m_yaw;   // Rotation around the Y axis
        float m_pitch; // Rotation around the X axis
    };

} // namespace flint