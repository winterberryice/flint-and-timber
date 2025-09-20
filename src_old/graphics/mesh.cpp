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
                // Front face
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},

                // Back face
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},

                // Top face
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},

                // Bottom face
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},

                // Right face
                {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},

                // Left face
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
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
