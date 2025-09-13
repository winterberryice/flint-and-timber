#pragma once

#include <glm/glm.hpp>

namespace flint
{
    namespace graphics
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec2 tex_coords;
            glm::vec3 normal;
            float light;
        };
    } // namespace graphics
} // namespace flint
