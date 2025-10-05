#include "selection_mesh.h"

#include <vector>
#include <iostream>

#include "../init/buffer.h"
#include "../vertex.h"

namespace flint::graphics
{

    SelectionMesh::SelectionMesh() = default;
    SelectionMesh::~SelectionMesh() = default;

    void SelectionMesh::generate(WGPUDevice device)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        const float thickness = 0.025f;

        // 8 corners of a cube
        const std::vector<glm::vec3> corners = {
            {0.0f, 0.0f, 0.0f}, // 0
            {1.0f, 0.0f, 0.0f}, // 1
            {1.0f, 1.0f, 0.0f}, // 2
            {0.0f, 1.0f, 0.0f}, // 3
            {0.0f, 0.0f, 1.0f}, // 4
            {1.0f, 0.0f, 1.0f}, // 5
            {1.0f, 1.0f, 1.0f}, // 6
            {0.0f, 1.0f, 1.0f}  // 7
        };

        // 12 edges defined by pairs of corner indices
        const std::vector<std::vector<int>> edges = {
            {0, 1}, {1, 5}, {5, 4}, {4, 0}, // Bottom face
            {3, 2}, {2, 6}, {6, 7}, {7, 3}, // Top face
            {0, 3}, {1, 2}, {4, 7}, {5, 6}  // Connecting edges
        };

        for (const auto &edge : edges)
        {
            glm::vec3 p1 = corners[edge[0]];
            glm::vec3 p2 = corners[edge[1]];

            uint32_t base_vertex_index = static_cast<uint32_t>(vertices.size());

            glm::vec3 dir = glm::normalize(p2 - p1);
            glm::vec3 tangent, bitangent;

            // Find a vector that is not collinear with dir to form a basis
            glm::vec3 c1 = glm::cross(dir, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 c2 = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));

            if (glm::length(c1) > glm::length(c2))
            {
                tangent = glm::normalize(c1);
            }
            else
            {
                tangent = glm::normalize(c2);
            }
            bitangent = glm::normalize(glm::cross(dir, tangent));

            // Scale by thickness
            tangent *= thickness / 2.0f;
            bitangent *= thickness / 2.0f;

            // Add 8 vertices for the thin box
            vertices.push_back({p1 - tangent - bitangent, {}, {}});
            vertices.push_back({p1 + tangent - bitangent, {}, {}});
            vertices.push_back({p1 + tangent + bitangent, {}, {}});
            vertices.push_back({p1 - tangent + bitangent, {}, {}});

            vertices.push_back({p2 - tangent - bitangent, {}, {}});
            vertices.push_back({p2 + tangent - bitangent, {}, {}});
            vertices.push_back({p2 + tangent + bitangent, {}, {}});
            vertices.push_back({p2 - tangent + bitangent, {}, {}});

            // Add 36 indices for the 12 triangles of the box
            std::vector<uint32_t> box_indices = {
                0, 1, 2, 0, 2, 3, // p1 face (near)
                4, 6, 5, 4, 7, 6, // p2 face (far)
                0, 4, 5, 0, 5, 1, // Bottom face
                1, 5, 6, 1, 6, 2, // Right face
                2, 6, 7, 2, 7, 3, // Top face
                3, 7, 4, 3, 4, 0  // Left face
            };

            for (uint32_t index : box_indices)
            {
                indices.push_back(base_vertex_index + index);
            }
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