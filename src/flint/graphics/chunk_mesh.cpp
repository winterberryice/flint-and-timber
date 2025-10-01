#include "chunk_mesh.hpp"
#include "../cube_geometry.h"
#include "../vertex.h"
#include <iostream>
#include <map>

namespace flint
{
    namespace graphics
    {
        // Define atlas properties. Atlas is 16x16 tiles.
        constexpr float ATLAS_WIDTH = 16.0f;
        constexpr float ATLAS_HEIGHT = 16.0f;
        constexpr float TILE_WIDTH = 1.0f / ATLAS_WIDTH;
        constexpr float TILE_HEIGHT = 1.0f / ATLAS_HEIGHT;

        // Sentinel color for grass top tinting in the shader.
        const glm::vec3 GRASS_TOP_TINT_COLOR = {0.1f, 0.9, 0.1f};

        // Texture coordinates for different block types.
        // We use a map for easy lookup. The key is BlockType, the value is another map
        // where the key is the face and the value is the (column, row) coordinate in the atlas.
        using TexCoords = glm::vec2;
        using FaceTextureMap = std::map<CubeGeometry::Face, TexCoords>;
        using BlockTextureMap = std::map<BlockType, FaceTextureMap>;

        const BlockTextureMap block_textures = {
            {BlockType::Grass, {
                {CubeGeometry::Face::Top,    TexCoords(0, 0)}, // Grayscale texture for tinting
                {CubeGeometry::Face::Bottom, TexCoords(2, 0)}, // Dirt
                {CubeGeometry::Face::Front,  TexCoords(3, 0)}, // Grass side
                {CubeGeometry::Face::Back,   TexCoords(3, 0)},
                {CubeGeometry::Face::Left,   TexCoords(3, 0)},
                {CubeGeometry::Face::Right,  TexCoords(3, 0)},
            }},
            {BlockType::Dirt, {
                {CubeGeometry::Face::Top,    TexCoords(2, 0)},
                {CubeGeometry::Face::Bottom, TexCoords(2, 0)},
                {CubeGeometry::Face::Front,  TexCoords(2, 0)},
                {CubeGeometry::Face::Back,   TexCoords(2, 0)},
                {CubeGeometry::Face::Left,   TexCoords(2, 0)},
                {CubeGeometry::Face::Right,  TexCoords(2, 0)},
            }},
        };

        TexCoords get_texture_coords(BlockType type, CubeGeometry::Face face) {
            auto block_it = block_textures.find(type);
            if (block_it != block_textures.end()) {
                auto face_it = block_it->second.find(face);
                if (face_it != block_it->second.end()) {
                    return face_it->second;
                }
            }
            return TexCoords(15, 15); // Default to a missing texture tile
        }


        ChunkMesh::ChunkMesh() : m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_device(nullptr), m_indexCount(0) {}

        ChunkMesh::~ChunkMesh()
        {
            cleanup();
        }

        void ChunkMesh::cleanup()
        {
            if (m_vertexBuffer) { wgpuBufferDestroy(m_vertexBuffer); wgpuBufferRelease(m_vertexBuffer); m_vertexBuffer = nullptr; }
            if (m_indexBuffer) { wgpuBufferDestroy(m_indexBuffer); wgpuBufferRelease(m_indexBuffer); m_indexBuffer = nullptr; }
            m_indexCount = 0;
        }

        void ChunkMesh::generate(WGPUDevice device, const flint::Chunk &chunk)
        {
            m_device = device;
            cleanup();

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
                        if (!currentBlock || !currentBlock->isSolid()) continue;

                        const std::array<CubeGeometry::Face, 6> &faces = CubeGeometry::getAllFaces();
                        const int neighborOffsets[6][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};

                        for (size_t i = 0; i < faces.size(); ++i)
                        {
                            CubeGeometry::Face face = faces[i];
                            int nx = x + neighborOffsets[i][0];
                            int ny = y + neighborOffsets[i][1];
                            int nz = z + neighborOffsets[i][2];

                            const Block *neighborBlock = chunk.getBlock(nx, ny, nz);
                            if (!neighborBlock || !neighborBlock->isSolid())
                            {
                                std::vector<flint::Vertex> faceVertices = CubeGeometry::getFaceVertices(face);
                                const std::vector<uint16_t> &faceIndices = CubeGeometry::getLocalFaceIndices();

                                TexCoords tile_coords = get_texture_coords(currentBlock->type, face);
                                glm::vec2 uv_offset = {tile_coords.x * TILE_WIDTH, tile_coords.y * TILE_HEIGHT};

                                glm::vec3 face_color = {1.0f, 1.0f, 1.0f}; // Default white
                                if (currentBlock->type == BlockType::Grass && face == CubeGeometry::Face::Top) {
                                    face_color = GRASS_TOP_TINT_COLOR;
                                }

                                for (const auto &vertex : faceVertices)
                                {
                                    glm::vec2 final_uv = {
                                        (vertex.tex_coords.x * TILE_WIDTH) + uv_offset.x,
                                        (vertex.tex_coords.y * TILE_HEIGHT) + uv_offset.y
                                    };

                                    vertices.push_back({
                                        {vertex.position.x + x, vertex.position.y + y, vertex.position.z + z},
                                        face_color,
                                        final_uv,
                                    });
                                }

                                for (const auto &index : faceIndices)
                                {
                                    indices.push_back(currentIndex + index);
                                }
                                currentIndex += 4;
                            }
                        }
                    }
                }
            }

            if (vertices.empty()) return;

            WGPUBufferDescriptor vertexBufferDesc = {};
            vertexBufferDesc.size = vertices.size() * sizeof(flint::Vertex);
            vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

            WGPUBufferDescriptor indexBufferDesc = {};
            indexBufferDesc.size = indices.size() * sizeof(uint16_t);
            indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, indices.data(), indexBufferDesc.size);

            m_indexCount = indices.size();
        }

        void ChunkMesh::render(WGPURenderPassEncoder renderPass) const
        {
            if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0) return;
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
        }
    }
}