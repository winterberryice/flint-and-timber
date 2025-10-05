#pragma once

#include "vertex.h"
#include <vector>
#include <cstdint>

namespace flint
{
    // A hardcoded 1x1x1 cube, centered at (0.5, 0.5, 0.5) to align with block coordinates.
    // We will translate it to the correct world position using the model matrix.
    const static float CUBE_SIZE = 1.0f;
    const static float CUBE_OFFSET = 0.5f;

    // We define 8 vertices for the cube.
    // We don't need normals or UVs for the wireframe, but our Vertex struct includes them,
    // so we provide dummy values.
    const static std::vector<Vertex> outline_vertices = {
        // Position                             // Normal (unused)      // UV (unused)
        {{CUBE_OFFSET, CUBE_OFFSET, CUBE_OFFSET}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},                 // 0
        {{CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET, CUBE_OFFSET}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},      // 1
        {{CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 2
        {{CUBE_OFFSET, CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},      // 3
        {{CUBE_OFFSET, CUBE_OFFSET, CUBE_OFFSET + CUBE_SIZE}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},      // 4
        {{CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET, CUBE_OFFSET + CUBE_SIZE}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 5
        {{CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET + CUBE_SIZE}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 6
        {{CUBE_OFFSET, CUBE_OFFSET + CUBE_SIZE, CUBE_OFFSET + CUBE_SIZE}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 7
    };

    // We define 12 lines using 24 indices.
    // WGPUPrimitiveTopology_LineList requires a separate pair of indices for each line.
    const static std::vector<uint16_t> outline_indices = {
        // Bottom face
        0, 1,
        1, 5,
        5, 4,
        4, 0,
        // Top face
        3, 2,
        2, 6,
        6, 7,
        7, 3,
        // Connecting lines
        0, 3,
        1, 2,
        4, 7,
        5, 6};

} // namespace flint