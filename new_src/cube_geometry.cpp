#include "cube_geometry.hpp"

namespace {
    const std::array<Vertex, 24> CUBE_VERTICES_DATA = {{
        // Front face
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Back face
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Right face
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        // Left face
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0, 0},
        // Top face
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        // Bottom face
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0, 0}
    }};

    const std::array<std::array<Vertex, 4>, 6> FACE_VERTICES = {{
        // Front
        {CUBE_VERTICES_DATA[0], CUBE_VERTICES_DATA[1], CUBE_VERTICES_DATA[2], CUBE_VERTICES_DATA[3]},
        // Back
        {CUBE_VERTICES_DATA[4], CUBE_VERTICES_DATA[5], CUBE_VERTICES_DATA[6], CUBE_VERTICES_DATA[7]},
        // Right
        {CUBE_VERTICES_DATA[8], CUBE_VERTICES_DATA[9], CUBE_VERTICES_DATA[10], CUBE_VERTICES_DATA[11]},
        // Left
        {CUBE_VERTICES_DATA[12], CUBE_VERTICES_DATA[13], CUBE_VERTICES_DATA[14], CUBE_VERTICES_DATA[15]},
        // Top
        {CUBE_VERTICES_DATA[16], CUBE_VERTICES_DATA[17], CUBE_VERTICES_DATA[18], CUBE_VERTICES_DATA[19]},
        // Bottom
        {CUBE_VERTICES_DATA[20], CUBE_VERTICES_DATA[21], CUBE_VERTICES_DATA[22], CUBE_VERTICES_DATA[23]}
    }};

    const std::array<uint16_t, 6> LOCAL_FACE_INDICES = {0, 1, 2, 0, 2, 3};
}

namespace CubeGeometry {
    const std::array<Vertex, 4>& get_face_vertices(CubeFace face) {
        return FACE_VERTICES[static_cast<int>(face)];
    }

    const std::array<uint16_t, 6>& get_face_indices() {
        return LOCAL_FACE_INDICES;
    }
}
