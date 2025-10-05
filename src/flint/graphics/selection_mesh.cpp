#include "selection_mesh.h"

#include "../init/buffer.h"
#include "../vertex.h"

#include <vector>
#include <iostream>

namespace flint::graphics
{

    SelectionMesh::SelectionMesh() = default;
    SelectionMesh::~SelectionMesh() = default;

    void SelectionMesh::generate(WGPUDevice device)
    {
        // Define the 8 vertices of a unit cube.
        std::vector<Vertex> vertices = {
            // Position, Normal (unused), UV (unused)
            {{0.0f, 0.0f, 0.0f}, {0, 0, 0}, {0, 0}}, // 0
            {{1.0f, 0.0f, 0.0f}, {0, 0, 0}, {0, 0}}, // 1
            {{1.0f, 0.0f, 1.0f}, {0, 0, 0}, {0, 0}}, // 2
            {{0.0f, 0.0f, 1.0f}, {0, 0, 0}, {0, 0}}, // 3
            {{0.0f, 1.0f, 0.0f}, {0, 0, 0}, {0, 0}}, // 4
            {{1.0f, 1.0f, 0.0f}, {0, 0, 0}, {0, 0}}, // 5
            {{1.0f, 1.0f, 1.0f}, {0, 0, 0}, {0, 0}}, // 6
            {{0.0f, 1.0f, 1.0f}, {0, 0, 0}, {0, 0}}, // 7
        };

        // Define the 12 edges (24 indices) of the cube.
        std::vector<uint32_t> indices = {
            // Bottom face
            0, 1, 1, 2, 2, 3, 3, 0,
            // Top face
            4, 5, 5, 6, 6, 7, 7, 4,
            // Vertical edges
            0, 4, 1, 5, 2, 6, 3, 7};

        m_indexCount = static_cast<uint32_t>(indices.size());

        if (m_vertexBuffer)
            wgpuBufferRelease(m_vertexBuffer);
        if (m_indexBuffer)
            wgpuBufferRelease(m_indexBuffer);

        m_vertexBuffer = init::create_vertex_buffer(device, "Selection Vertex Buffer", vertices.data(), vertices.size() * sizeof(Vertex));
        m_indexBuffer = init::create_index_buffer(device, "Selection Index Buffer", indices.data(), indices.size() * sizeof(uint32_t));
    }

    void SelectionMesh::render(WGPURenderPassEncoder renderPass) const
    {
        if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0)
        {
            return;
        }
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
    }

    void SelectionMesh::cleanup()
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
        m_indexCount = 0;
    }

} // namespace flint::graphics