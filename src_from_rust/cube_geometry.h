#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <array>
#include <cstdint>

namespace flint {

    // TODO: This is a placeholder definition.
    // The actual Vertex struct will be defined when translating the corresponding file.
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
        uint32_t tree_id;
        uint32_t sky_light;
    };

    enum class CubeFace {
        Front,  // Negative Z
        Back,   // Positive Z
        Right,  // Positive X
        Left,   // Negative X
        Top,    // Positive Y
        Bottom, // Negative Y
    };

    class CubeGeometry {
    public:
        // Returns a slice of 4 vertices for a given face.
        static std::vector<Vertex> get_vertices_template(CubeFace face);

        // Returns the 6 indices for a single quad face.
        static const std::array<uint16_t, 6>& get_local_indices();
    };

} // namespace flint
