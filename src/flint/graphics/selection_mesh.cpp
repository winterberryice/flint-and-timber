#include "selection_mesh.h"

#include "../init/buffer.h"
#include "../vertex.h"

#include <vector>
#include <iostream>

namespace flint::graphics
{

    namespace
    {
        // Helper function to generate a cuboid mesh for a thick line segment.
        void add_cuboid(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, const glm::vec3 &min, const glm::vec3 &max)
        {
            uint32_t base_vertex = vertices.size();

            // Add 8 vertices for the cuboid.
            vertices.push_back({{min.x, min.y, min.z}, {}, {}});
            vertices.push_back({{max.x, min.y, min.z}, {}, {}});
            vertices.push_back({{max.x, min.y, max.z}, {}, {}});
            vertices.push_back({{min.x, min.y, max.z}, {}, {}});
            vertices.push_back({{min.x, max.y, min.z}, {}, {}});
            vertices.push_back({{max.x, max.y, min.z}, {}, {}});
            vertices.push_back({{max.x, max.y, max.z}, {}, {}});
            vertices.push_back({{min.x, max.y, max.z}, {}, {}});

            // Add 36 indices (12 triangles) for the cuboid.
            std::vector<uint32_t> cube_indices = {
                0, 1, 2, 0, 2, 3, // Bottom
                4, 5, 6, 4, 6, 7, // Top
                0, 4, 7, 0, 7, 3, // Left
                1, 2, 6, 1, 6, 5, // Right
                0, 1, 5, 0, 5, 4, // Back
                3, 7, 6, 3, 6, 2  // Front
            };

            for (uint32_t index : cube_indices)
            {
                indices.push_back(base_vertex + index);
            }
        }
    }

    SelectionMesh::SelectionMesh() = default;
    SelectionMesh::~SelectionMesh() = default;

    void SelectionMesh::generate(WGPUDevice device)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        const float t = 0.02f; // Half the thickness of the lines

        // Create 12 cuboids for the edges of a unit cube.
        // Edges along X
        add_cuboid(vertices, indices, {0.0f, 0.0f, 0.0f}, {1.0f, 2 * t, 2 * t});
        add_cuboid(vertices, indices, {0.0f, 1.0f - 2 * t, 0.0f}, {1.0f, 1.0f, 2 * t});
        add_cuboid(vertices, indices, {0.0f, 0.0f, 1.0f - 2 * t}, {1.0f, 2 * t, 1.0f});
        add_cuboid(vertices, indices, {0.0f, 1.0f - 2 * t, 1.0f - 2 * t}, {1.0f, 1.0f, 1.0f});

        // Edges along Y
        add_cuboid(vertices, indices, {0.0f, 0.0f, 0.0f}, {2 * t, 1.0f, 2 * t});
        add_cuboid(vertices, indices, {1.0f - 2 * t, 0.0f, 0.0f}, {1.0f, 1.0f, 2 * t});
        add_cuboid(vertices, indices, {0.0f, 0.0f, 1.0f - 2 * t}, {2 * t, 1.0f, 1.0f});
        add_cuboid(vertices, indices, {1.0f - 2 * t, 0.0f, 1.0f - 2 * t}, {1.0f, 1.0f, 1.0f});

        // Edges along Z
        add_cuboid(vertices, indices, {0.0f, 0.0f, 0.0f}, {2 * t, 2 * t, 1.0f});
        add_cuboid(vertices, indices, {1.0f - 2 * t, 0.0f, 0.0f}, {1.0f, 2 * t, 1.0f});
        add_cuboid(vertices, indices, {0.0f, 1.0f - 2 * t, 0.0f}, {2 * t, 1.0f, 1.0f});
        add_cuboid(vertices, indices, {1.0f - 2 * t, 1.0f - 2 * t, 0.0f}, {1.0f, 1.0f, 1.0f});

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