#pragma once

#include "vertex.h"
#include <vector>
#include <array>
#include <cstdint> // For uint16_t

namespace flint
{
    namespace CubeGeometry
    {

        // Enum to identify cube faces.
        enum class Face
        {
            Front,  // Negative Z
            Back,   // Positive Z
            Right,  // Positive X
            Left,   // Negative X
            Top,    // Positive Y
            Bottom, // Negative Y
        };

        // Returns a reference to a static vector containing all 24 vertices.
        // Returning a const reference is efficient as it avoids copying the entire vector.
        const std::vector<Vertex> &getVertices();

        // Returns a reference to a static vector containing all 36 indices.
        const std::vector<uint16_t> &getIndices();

        // Returns a *new* vector containing the 4 vertices for a specific face.
        // This involves a small copy but is a safe alternative to returning a raw pointer.
        std::vector<Vertex> getFaceVertices(Face face);

        // Returns a reference to a static vector containing the 6 indices for a single quad.
        const std::vector<uint16_t> &getLocalFaceIndices();

        // Returns a reference to an array containing all 6 face types.
        const std::array<Face, 6> &getAllFaces();

    } // namespace CubeGeometry
} // namespace flint