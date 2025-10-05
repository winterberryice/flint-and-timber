#include "selection_mesh.h"

#include "../init/buffer.h"
#include "../cube_geometry.h"
#include "../vertex.h"

#include <vector>
#include <iostream>

namespace flint::graphics
{

    SelectionMesh::SelectionMesh() = default;
    SelectionMesh::~SelectionMesh() = default;

    void SelectionMesh::generate(WGPUDevice device)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        const auto &cube_vertices = flint::CubeGeometry::getVertices();
        const auto &cube_indices_16 = flint::CubeGeometry::getIndices();

        const float scale = 1.01f;
        const float offset = -0.005f;

        for (const auto &vert : cube_vertices)
        {
            glm::vec3 scaled_pos = vert.position * scale + glm::vec3(offset, offset, offset);
            vertices.push_back({scaled_pos, vert.color, vert.uv});
        }

        indices.reserve(cube_indices_16.size());
        for (uint16_t index : cube_indices_16)
        {
            indices.push_back(static_cast<uint32_t>(index));
        }

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