#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;
    uint32_t tree_id;
    uint32_t sky_light;
};
