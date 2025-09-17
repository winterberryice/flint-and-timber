#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace flint {
    namespace graphics {

        struct Vertex {
            glm::vec3 position;
            glm::vec3 color;
            uint32_t sky_light;
        };

        struct ChunkMeshData {
            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;
        };

    } // namespace graphics
} // namespace flint
