#include "mesh.hpp"
#include <iostream>

namespace flint
{
    namespace graphics
    {

        CubeMesh::CubeMesh()
        {
            createCubeData();
        }

        CubeMesh::~CubeMesh()
        {
            cleanup();
        }

        void CubeMesh::createCubeData()
        {
            // Cube vertices (8 corners)
            // Each face will have different colors for easy identification
            m_vertices = {
                // Front face (red)
                {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
                {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},  // 1
                {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},   // 2
                {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},  // 3

                // Back face (green)
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 4
                {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // 5
                {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},   // 6
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // 7

                // Top face (blue)
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},  // 8
                {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},   // 9
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},  // 10
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 11

                // Bottom face (yellow)
                {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},  // 12
                {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},   // 13
                {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},  // 14
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 15

                // Right face (magenta)
                {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},  // 16
                {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 17
                {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},  // 18
                {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},   // 19

                // Left face (cyan)
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},  // 20
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 21
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},  // 22
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},   // 23
            };

            // Indices for triangles (2 triangles per face, 6 faces)
            m_indices = {
                // Front face
                0, 1, 2, 2, 3, 0,
                // Back face
                4, 6, 5, 6, 4, 7,
                // Top face
                8, 9, 10, 10, 11, 8,
                // Bottom face
                12, 14, 13, 14, 12, 15,
                // Right face
                16, 17, 18, 18, 19, 16,
                // Left face
                20, 22, 21, 22, 20, 23};
        }

        bool CubeMesh::initialize(WGPUDevice device)
        {
            m_device = device;

            // Create vertex buffer
            WGPUBufferDescriptor vertexBufferDesc = {};
            vertexBufferDesc.size = m_vertices.size() * sizeof(Vertex);
            vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            vertexBufferDesc.mappedAtCreation = false;

            m_vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
            if (!m_vertexBuffer)
            {
                std::cerr << "Failed to create vertex buffer" << std::endl;
                return false;
            }

            // Upload vertex data
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_vertexBuffer, 0,
                                 m_vertices.data(), vertexBufferDesc.size);

            // Create index buffer
            WGPUBufferDescriptor indexBufferDesc = {};
            indexBufferDesc.size = m_indices.size() * sizeof(uint16_t);
            indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            indexBufferDesc.mappedAtCreation = false;

            m_indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);
            if (!m_indexBuffer)
            {
                std::cerr << "Failed to create index buffer" << std::endl;
                return false;
            }

            // Upload index data
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_indexBuffer, 0,
                                 m_indices.data(), indexBufferDesc.size);

            return true;
        }

        void CubeMesh::cleanup()
        {
            if (m_vertexBuffer)
            {
                wgpuBufferRelease(m_vertexBuffer);
                m_vertexBuffer = nullptr;
            }
            if (m_indexBuffer)
            {
                wgpuBufferRelease(m_indexBuffer);
                m_indexBuffer = nullptr;
            }
        }

        void CubeMesh::render(WGPURenderPassEncoder renderPass) const
        {
            if (!m_vertexBuffer || !m_indexBuffer)
                return;

            // Bind buffers and draw
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        }

    } // namespace graphics
} // namespace flint
