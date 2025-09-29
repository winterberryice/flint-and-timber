#include "debug_mesh.hpp"
#include <vector>
#include <string.h> // For memcpy

namespace flint
{
    namespace graphics
    {
        DebugMesh::DebugMesh() : m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_indexCount(0) {}

        void DebugMesh::generate(WGPUDevice device)
        {
            // A simple unit cube centered at (0,0,0).
            // It can be scaled and translated by a model matrix to match any AABB.
            const std::vector<float> vertices = {
                // Position (x, y, z)
                -0.5f, -0.5f, -0.5f, // 0
                 0.5f, -0.5f, -0.5f, // 1
                 0.5f,  0.5f, -0.5f, // 2
                -0.5f,  0.5f, -0.5f, // 3
                -0.5f, -0.5f,  0.5f, // 4
                 0.5f, -0.5f,  0.5f, // 5
                 0.5f,  0.5f,  0.5f, // 6
                -0.5f,  0.5f,  0.5f, // 7
            };

            const std::vector<uint16_t> indices = {
                // Front face
                0, 1, 2, 2, 3, 0,
                // Back face
                4, 5, 6, 6, 7, 4,
                // Left face
                4, 0, 3, 3, 7, 4,
                // Right face
                1, 5, 6, 6, 2, 1,
                // Top face
                3, 2, 6, 6, 7, 3,
                // Bottom face
                4, 5, 1, 1, 0, 4,
            };

            m_indexCount = static_cast<uint32_t>(indices.size());

            // Create vertex buffer
            WGPUBufferDescriptor vertexBufferDesc = {
                .label = "Debug Vertex Buffer",
                .usage = WGPUBufferUsage_Vertex,
                .size = vertices.size() * sizeof(float),
                .mappedAtCreation = true,
            };
            m_vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
            memcpy(wgpuBufferGetMappedRange(m_vertexBuffer, 0, WGPU_WHOLE_SIZE), vertices.data(), vertices.size() * sizeof(float));
            wgpuBufferUnmap(m_vertexBuffer);

            // Create index buffer
            WGPUBufferDescriptor indexBufferDesc = {
                .label = "Debug Index Buffer",
                .usage = WGPUBufferUsage_Index,
                .size = indices.size() * sizeof(uint16_t),
                .mappedAtCreation = true,
            };
            m_indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);
            memcpy(wgpuBufferGetMappedRange(m_indexBuffer, 0, WGPU_WHOLE_SIZE), indices.data(), indices.size() * sizeof(uint16_t));
            wgpuBufferUnmap(m_indexBuffer);
        }

        void DebugMesh::cleanup()
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

        void DebugMesh::render(WGPURenderPassEncoder renderPass) const
        {
            if (!m_vertexBuffer || !m_indexBuffer)
                return;

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
        }
    }
}