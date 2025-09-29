#include "chunk_mesh.hpp"
#include "../cube_geometry.h"
#include "../vertex.h"
#include <iostream>

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
                        if (!currentBlock || !currentBlock->isSolid())
                        {
                            continue;
                        }

                        // Define colors based on block type
                        glm::vec4 color;
                        if (currentBlock->type == BlockType::Grass)
                        {
                            color = {0.0f, 1.0f, 0.0f, 1.0f}; // Green
                        }
                        else
                        { // Dirt
                            color = {0.5f, 0.25f, 0.0f, 1.0f}; // Brown
                        }

                        // Check neighbors
                        const std::array<CubeGeometry::Face, 6> &faces = CubeGeometry::getAllFaces();
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
                                // Add face to the mesh
                                std::vector<flint::Vertex> faceVertices = CubeGeometry::getFaceVertices(faces[i]);
                                const std::vector<uint16_t> &faceIndices = CubeGeometry::getLocalFaceIndices();

                                for (const auto &vertex : faceVertices)
                                {
                                    vertices.push_back({
                                        {vertex.position.x + x, vertex.position.y + y, vertex.position.z + z},
                                        color,
                                    });
                                }

                                for (const auto &index : faceIndices)
                                {
                                    indices.push_back(currentIndex + index);
                                }
                                currentIndex += 4; // Each face has 4 vertices
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