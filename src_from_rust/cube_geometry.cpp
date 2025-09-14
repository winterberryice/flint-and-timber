#include "cube_geometry.h"

namespace flint {

    // 24 unique vertices for a cube, 4 for each face.
    const std::array<Vertex, 24> CUBE_VERTICES_DATA = {{
        // Front face (-Z)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Back face (+Z)
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Right face (+X)
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        // Left face (-X)
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Top face (+Y)
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        // Bottom face (-Y)
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0}
    }};

    const std::array<uint16_t, 6> LOCAL_FACE_INDICES = {{0, 1, 2, 0, 2, 3}};

    const std::array<size_t, 6> FACE_VERTEX_START_INDICES = {{
        0,  // Front
        4,  // Back
        8,  // Right
        12, // Left
        16, // Top
        20  // Bottom
    }};

    constexpr size_t NUM_VERTICES_PER_FACE = 4;

    std::vector<Vertex> CubeGeometry::get_vertices_template(CubeFace face) {
        size_t start_index = FACE_VERTEX_START_INDICES[static_cast<int>(face)];
        std::vector<Vertex> vertices;
        vertices.reserve(NUM_VERTICES_PER_FACE);
        for(size_t i = 0; i < NUM_VERTICES_PER_FACE; ++i) {
            vertices.push_back(CUBE_VERTICES_DATA[start_index + i]);
        }
        return vertices;
    }

    const std::array<uint16_t, 6>& CubeGeometry::get_local_indices() {
        return LOCAL_FACE_INDICES;
    }

} // namespace flint
