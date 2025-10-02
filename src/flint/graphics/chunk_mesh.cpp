#include "chunk_mesh.hpp"
#include "../cube_geometry.h"
#include "../vertex.h"
#include <iostream>
#include <array>

namespace
{
    // The texture atlas is a 16x1 grid of tiles.
    const float ATLAS_COLS = 16.0f;
    const float ATLAS_ROWS = 1.0f;

    struct FaceTextureInfo
    {
        std::array<glm::vec2, 4> uvs;
        glm::vec3 color;
    };

    // This function determines the texture coordinates and color for a given block face.
    FaceTextureInfo get_face_texture_info(flint::BlockType block_type, flint::CubeGeometry::Face face)
    {
        glm::ivec2 tile_coords;

        // Default color is white (no tint).
        glm::vec3 color = {1.0f, 1.0f, 1.0f};

        switch (block_type)
        {
        case flint::BlockType::Grass:
            if (face == flint::CubeGeometry::Face::Top)
            {
                tile_coords = {0, 0}; // Grass top
                // Use the sentinel color to signal the shader to apply a tint.
                color = {0.1f, 0.9f, 0.1f};
            }
            else if (face == flint::CubeGeometry::Face::Bottom)
            {
                tile_coords = {2, 0}; // Dirt
            }
            else
            {
                tile_coords = {1, 0}; // Grass side
            }
            break;
        case flint::BlockType::Dirt:
            tile_coords = {2, 0}; // Dirt
            break;
        case flint::BlockType::OakLog:
            if (face == flint::CubeGeometry::Face::Top || face == flint::CubeGeometry::Face::Bottom)
            {
                tile_coords = {5, 0}; // Oak log top/bottom
            }
            else
            {
                tile_coords = {4, 0}; // Oak log side
            }
            break;
        case flint::BlockType::OakLeaves:
            tile_coords = {6, 0}; // Oak leaves
            break;
        default:
            tile_coords = {2, 0}; // Dirt
            break;
        }

        // Calculate the size of a single texture tile in UV space.
        float tile_width = 1.0f / ATLAS_COLS;
        float tile_height = 1.0f / ATLAS_ROWS;

        // Calculate UV coordinates from tile coordinates.
        float u0 = static_cast<float>(tile_coords.x) * tile_width;
        float v0 = static_cast<float>(tile_coords.y) * tile_height;
        float u1 = u0 + tile_width;
        float v1 = v0 + tile_height;

        std::array<glm::vec2, 4> uvs;
        switch (face)
        {
        // The UV coordinates must match the vertex order for each face defined in cube_geometry.cpp
        case flint::CubeGeometry::Face::Front: // Vertices: BL, TL, TR, BR
            uvs = {{{u0, v1}, {u0, v0}, {u1, v0}, {u1, v1}}};
            break;
        case flint::CubeGeometry::Face::Back: // Vertices: BL, BR, TR, TL -> Texture is mirrored
            uvs = {{{u1, v1}, {u0, v1}, {u0, v0}, {u1, v0}}};
            break;
        case flint::CubeGeometry::Face::Right: // Vertices: BL, TL, TR, BR
            uvs = {{{u0, v1}, {u0, v0}, {u1, v0}, {u1, v1}}};
            break;
        case flint::CubeGeometry::Face::Left: // Vertices: BL, TL, TR, BR -> Texture is mirrored
            uvs = {{{u1, v1}, {u1, v0}, {u0, v0}, {u0, v1}}};
            break;
        case flint::CubeGeometry::Face::Top: // Vertices: BL, BR, TR, TL
            uvs = {{{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}}};
            break;
        case flint::CubeGeometry::Face::Bottom: // Vertices: TL, BL, BR, TR
            uvs = {{{u0, v0}, {u0, v1}, {u1, v1}, {u1, v0}}};
            break;
        }

        return {.uvs = uvs, .color = color};
    }
} // namespace

namespace flint
{
    namespace graphics
    {

        ChunkMesh::ChunkMesh() : m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_device(nullptr), m_indexCount(0) {}

        ChunkMesh::~ChunkMesh()
        {
            cleanup();
        }

        void ChunkMesh::cleanup()
        {
            if (m_vertexBuffer)
            {
                wgpuBufferDestroy(m_vertexBuffer);
                wgpuBufferRelease(m_vertexBuffer);
                m_vertexBuffer = nullptr;
            }
            if (m_indexBuffer)
            {
                wgpuBufferDestroy(m_indexBuffer);
                wgpuBufferRelease(m_indexBuffer);
                m_indexBuffer = nullptr;
            }
            m_indexCount = 0;
        }

        void ChunkMesh::generate(WGPUDevice device, const flint::Chunk &chunk)
        {
            m_device = device;
            cleanup(); // Clean up existing buffers before generating new ones.

            std::vector<flint::Vertex> vertices;
            std::vector<uint16_t> indices;
            uint16_t currentIndex = 0;

            for (size_t x = 0; x < CHUNK_WIDTH; ++x)
            {
                for (size_t y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    for (size_t z = 0; z < CHUNK_DEPTH; ++z)
                    {
                        const Block *currentBlock = chunk.getBlock(x, y, z);
                        if (!currentBlock || currentBlock->type == BlockType::Air)
                        {
                            continue;
                        }

                        const auto &faces = CubeGeometry::getAllFaces();

                        // The neighbor offsets need to match the order of faces in `getAllFaces`:
                        // Front, Back, Right, Left, Top, Bottom.
                        const int neighborOffsets[6][3] = {
                            {0, 0, -1}, // Front
                            {0, 0, 1},  // Back
                            {1, 0, 0},  // Right
                            {-1, 0, 0}, // Left
                            {0, 1, 0},  // Top
                            {0, -1, 0}  // Bottom
                        };

                        for (size_t i = 0; i < faces.size(); ++i)
                        {
                            int nx = x + neighborOffsets[i][0];
                            int ny = y + neighborOffsets[i][1];
                            int nz = z + neighborOffsets[i][2];

                            const Block *neighborBlock = chunk.getBlock(nx, ny, nz);

                            if (!neighborBlock || !neighborBlock->isSolid())
                            {
                                // This face is visible, add it to the mesh.
                                auto face_info = get_face_texture_info(currentBlock->type, faces[i]);
                                std::vector<flint::Vertex> faceVertices = CubeGeometry::getFaceVertices(faces[i]);
                                const std::vector<uint16_t> &faceIndices = CubeGeometry::getLocalFaceIndices();

                                for (size_t j = 0; j < faceVertices.size(); ++j)
                                {
                                    vertices.push_back({
                                        // Offset the vertex position by the block's position in the chunk.
                                        .position = faceVertices[j].position + glm::vec3(x, y, z),
                                        // The color is now used for lighting/tinting.
                                        .color = face_info.color,
                                        // Assign the UV coordinates for this vertex.
                                        .uv = face_info.uvs[j],
                                    });
                                }

                                for (const auto &index : faceIndices)
                                {
                                    indices.push_back(currentIndex + index);
                                }
                                currentIndex += 4; // Each face has 4 vertices.
                            }
                        }
                    }
                }
            }

            if (vertices.empty() || indices.empty())
            {
                std::cout << "Chunk mesh is empty, nothing to render." << std::endl;
                return;
            }

            // Create vertex buffer
            WGPUBufferDescriptor vertexBufferDesc = {};
            vertexBufferDesc.size = vertices.size() * sizeof(flint::Vertex);
            vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            vertexBufferDesc.mappedAtCreation = false;
            m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

            // Create index buffer
            WGPUBufferDescriptor indexBufferDesc = {};
            indexBufferDesc.size = indices.size() * sizeof(uint16_t);
            indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            indexBufferDesc.mappedAtCreation = false;
            m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, indices.data(), indexBufferDesc.size);

            m_indexCount = indices.size();
        }

        void ChunkMesh::render(WGPURenderPassEncoder renderPass) const
        {
            if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0)
            {
                return; // Nothing to render
            }

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
        }

    } // namespace graphics
} // namespace flint