#pragma once

#include "vertex.hpp"
#include <vector>
#include <array>

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

    namespace CubeGeometry
    {
        const std::array<Vertex, 4> &get_face_vertices(CubeFace face);
        const std::array<uint16_t, 6> &get_face_indices();
    }
} // namespace flint
