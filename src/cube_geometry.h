#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <array>
#include <cstdint>

#include "vertex.h"

namespace flint
{

    enum class CubeFace
    {
        Front,  // Negative Z
        Back,   // Positive Z
        Right,  // Positive X
        Left,   // Negative X
        Top,    // Positive Y
        Bottom, // Negative Y
    };

    class CubeGeometry
    {
    public:
        // Returns a slice of 4 vertices for a given face.
        static std::vector<Vertex> get_vertices_template(CubeFace face);

        // Returns the 6 indices for a single quad face.
        static const std::array<uint16_t, 6> &get_local_indices();
    };

} // namespace flint
