#include "cube_geometry.h"

namespace flint
{
    namespace CubeGeometry
    {

        // We define the large data arrays inside an anonymous namespace
        // to limit their visibility to just this file.
        namespace
        {
            // We use std::array for the raw data as it's a fixed-size, compile-time structure.
            // The cube is defined in the [0, 1] range, with its origin at the minimum corner.
            // Each face has 4 vertices. The UV coordinates map a single texture tile to the face.
            // UV origin (0,0) is top-left of the texture.
            // The winding order is counter-clockwise (CCW).
            constexpr std::array<Vertex, 24> CUBE_VERTICES_DATA = {{
                // Front face (Z = 0.0)
                {.position = {0.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-left
                {.position = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-left
                {.position = {1.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-right
                {.position = {1.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-right

                // Back face (Z = 1.0)
                {.position = {0.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-left
                {.position = {1.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-right
                {.position = {1.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-right
                {.position = {0.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-left

                // Right face (X = 1.0)
                {.position = {1.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-left
                {.position = {1.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-left
                {.position = {1.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-right
                {.position = {1.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-right

                // Left face (X = 0.0)
                {.position = {0.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-left
                {.position = {0.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-left
                {.position = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-right
                {.position = {0.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-right

                // Top face (Y = 1.0)
                {.position = {0.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-left
                {.position = {1.0f, 1.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-right
                {.position = {1.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-right
                {.position = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-left

                // Bottom face (Y = 0.0)
                {.position = {0.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 0.0f}}, // Top-left
                {.position = {0.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {1.0f, 1.0f}}, // Bottom-left
                {.position = {1.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 1.0f}}, // Bottom-right
                {.position = {1.0f, 0.0f, 1.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coords = {0.0f, 0.0f}}, // Top-right
            }};

            constexpr std::array<uint16_t, 36> CUBE_INDICES_DATA = {{
                0, 1, 2, 0, 2, 3,       // Front
                4, 5, 6, 4, 6, 7,       // Back
                8, 9, 10, 8, 10, 11,    // Right
                12, 13, 14, 12, 14, 15, // Left
                16, 17, 18, 16, 18, 19, // Top
                20, 21, 22, 20, 22, 23  // Bottom
            }};

            constexpr std::array<uint16_t, 6> LOCAL_FACE_INDICES = {{0, 1, 2, 0, 2, 3}};
            constexpr std::array<Face, 6> ALL_FACES = {{Face::Front, Face::Back, Face::Right, Face::Left, Face::Top, Face::Bottom}};
        } // anonymous namespace

        const std::vector<Vertex> &getVertices()
        {
            // This static vector is initialized from the raw data array only once,
            // the first time this function is called.
            static const std::vector<Vertex> vertices(CUBE_VERTICES_DATA.begin(), CUBE_VERTICES_DATA.end());
            return vertices;
        }

        const std::vector<uint16_t> &getIndices()
        {
            // Same pattern for the indices.
            static const std::vector<uint16_t> indices(CUBE_INDICES_DATA.begin(), CUBE_INDICES_DATA.end());
            return indices;
        }

        std::vector<Vertex> getFaceVertices(Face face)
        {
            constexpr size_t vertices_per_face = 4;
            size_t start_index = static_cast<size_t>(face) * vertices_per_face;

            // Create and return a new vector containing just the 4 vertices for the requested face.
            return {
                CUBE_VERTICES_DATA[start_index],
                CUBE_VERTICES_DATA[start_index + 1],
                CUBE_VERTICES_DATA[start_index + 2],
                CUBE_VERTICES_DATA[start_index + 3]};
        }

        const std::vector<uint16_t> &getLocalFaceIndices()
        {
            static const std::vector<uint16_t> indices(LOCAL_FACE_INDICES.begin(), LOCAL_FACE_INDICES.end());
            return indices;
        }

        const std::array<Face, 6> &getAllFaces()
        {
            return ALL_FACES;
        }

    } // namespace CubeGeometry
} // namespace flint