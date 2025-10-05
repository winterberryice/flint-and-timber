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
            // The vertices must be wound in a counter-clockwise order to be considered
            // front-facing by the default rasterizer settings (cull_mode = "back").
            std::vector<uint32_t> cube_indices = {
                // Bottom face (-Y)
                0, 3, 2, 0, 2, 1,
                // Top face (+Y)
                4, 5, 6, 4, 6, 7,
                // Left face (-X)
                0, 4, 7, 0, 7, 3,
                // Right face (+X)
                1, 5, 6, 1, 6, 2,
                // Back face (-Z)
                0, 1, 5, 0, 5, 4,
                // Front face (+Z)
                3, 2, 6, 3, 6, 7
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

        const float t = 0.015f; // Half the thickness of the lines

        // A unit cube is from (0,0,0) to (1,1,1). The 12 cuboids are centered on its edges.

        // 4 edges along the X axis
        add_cuboid(vertices, indices, {0.0f, -t, -t}, {1.0f, t, t});
        add_cuboid(vertices, indices, {0.0f, 1.0f - t, -t}, {1.0f, 1.0f + t, t});
        add_cuboid(vertices, indices, {0.0f, -t, 1.0f - t}, {1.0f, t, 1.0f + t});
        add_cuboid(vertices, indices, {0.0f, 1.0f - t, 1.0f - t}, {1.0f, 1.0f + t, 1.0f + t});

        // 4 edges along the Y axis
        add_cuboid(vertices, indices, {-t, 0.0f, -t}, {t, 1.0f, t});
        add_cuboid(vertices, indices, {1.0f - t, 0.0f, -t}, {1.0f + t, 1.0f, t});
        add_cuboid(vertices, indices, {-t, 0.0f, 1.0f - t}, {t, 1.0f, 1.0f + t});
        add_cuboid(vertices, indices, {1.0f - t, 0.0f, 1.0f - t}, {1.0f + t, 1.0f, 1.0f + t});

        // 4 edges along the Z axis
        add_cuboid(vertices, indices, {-t, -t, 0.0f}, {t, t, 1.0f});
        add_cuboid(vertices, indices, {1.0f - t, -t, 0.0f}, {1.0f + t, t, 1.0f});
        add_cuboid(vertices, indices, {-t, 1.0f - t, 0.0f}, {t, 1.0f + t, 1.0f});
        add_cuboid(vertices, indices, {1.0f - t, 1.0f - t, 0.0f}, {1.0f + t, 1.0f + t, 1.0f});

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