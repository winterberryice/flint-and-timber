#pragma once

#include <glm/glm.hpp>

struct CameraUniform {
    glm::mat4 view_proj;

    CameraUniform() : view_proj(glm::mat4(1.0f)) {}
};
