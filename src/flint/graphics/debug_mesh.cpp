#include "debug_mesh.hpp"
#include <string.h> // For memcpy

namespace flint
{
    namespace graphics
    {
        DebugMesh::DebugMesh() : m_device(nullptr), m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_indexCount(0) {}

        DebugMesh::~DebugMesh()
        {
            cleanup();
        }

        void DebugMesh::cleanup()
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

        void DebugMesh::generate(WGPUDevice device, const physics::AABB &aabb, const glm::vec4 &color)
        {
            m_device = device;
            cleanup(); // Ensure old buffers are released before creating new ones.

            // Define the 8 corners of the AABB
            const std::vector<flint::Vertex> vertices = {
                {glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z), color},
                {glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z), color},
                {glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z), color},
                {glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z), color},
                {glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z), color},
                {glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z), color},
                {glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z), color},
                {glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z), color},
            };

            const std::vector<uint16_t> indices = {
                0, 1, 2, 2, 3, 0, // Front
                4, 7, 6, 6, 5, 4, // Back
                4, 3, 0, 0, 7, 4, // Left
                1, 2, 6, 6, 5, 1, // Right
                3, 6, 2, 2, 7, 3, // Top
                0, 5, 4, 4, 1, 0  // Bottom
            };

            m_indexCount = static_cast<uint32_t>(indices.size());

            if (vertices.empty() || indices.empty())
            {
                return;
            }

            // Create vertex buffer
            WGPUBufferDescriptor vertexBufferDesc = {};
            vertexBufferDesc.size = vertices.size() * sizeof(flint::Vertex);
            vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            vertexBufferDesc.mappedAtCreation = false; // We will use wgpuQueueWriteBuffer
            m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

            // Create index buffer
            WGPUBufferDescriptor indexBufferDesc = {};
            indexBufferDesc.size = indices.size() * sizeof(uint16_t);
            indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            indexBufferDesc.mappedAtCreation = false;
            m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, indices.data(), indexBufferDesc.size);
        }

        void DebugMesh::render(WGPURenderPassEncoder renderPass) const
        {
            if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0)
            {
                return;
            }

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
        }
    }
}