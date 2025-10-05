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
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 color = {0.6f, 0.6f, 0.6f}; // A nice gray color.
        float border = 0.05f;                 // The thickness of the border.

        auto create_face_border = [&](
            const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3)
        {
            uint32_t base_vertex = static_cast<uint32_t>(vertices.size());

            // Add the 4 outer vertices of the face.
            vertices.push_back({p0, color, {}});
            vertices.push_back({p1, color, {}});
            vertices.push_back({p2, color, {}});
            vertices.push_back({p3, color, {}});

            // Add the 4 inner vertices of the face, inset by the border amount.
            // For an axis-aligned quad, moving a vertex by 'border' units along each of the two
            // adjacent edges correctly creates a uniform inset. For example, for p0, we move it
            // along the edge towards p1 and along the edge towards p3. This creates a visually
            // consistent border thickness.
            glm::vec3 i0 = p0 + (p1 - p0) * border + (p3 - p0) * border;
            glm::vec3 i1 = p1 + (p0 - p1) * border + (p2 - p1) * border;
            glm::vec3 i2 = p2 + (p1 - p2) * border + (p3 - p2) * border;
            glm::vec3 i3 = p3 + (p0 - p3) * border + (p2 - p3) * border;
            vertices.push_back({i0, color, {}});
            vertices.push_back({i1, color, {}});
            vertices.push_back({i2, color, {}});
            vertices.push_back({i3, color, {}});

            // Add indices to create 8 triangles that form the border.
            // Bottom edge
            indices.push_back(base_vertex + 0);
            indices.push_back(base_vertex + 1);
            indices.push_back(base_vertex + 4);
            indices.push_back(base_vertex + 1);
            indices.push_back(base_vertex + 5);
            indices.push_back(base_vertex + 4);
            // Right edge
            indices.push_back(base_vertex + 1);
            indices.push_back(base_vertex + 2);
            indices.push_back(base_vertex + 5);
            indices.push_back(base_vertex + 2);
            indices.push_back(base_vertex + 6);
            indices.push_back(base_vertex + 5);
            // Top edge
            indices.push_back(base_vertex + 2);
            indices.push_back(base_vertex + 3);
            indices.push_back(base_vertex + 6);
            indices.push_back(base_vertex + 3);
            indices.push_back(base_vertex + 7);
            indices.push_back(base_vertex + 6);
            // Left edge
            indices.push_back(base_vertex + 3);
            indices.push_back(base_vertex + 0);
            indices.push_back(base_vertex + 7);
            indices.push_back(base_vertex + 0);
            indices.push_back(base_vertex + 4);
            indices.push_back(base_vertex + 7);
        };

        // Generate the border for each of the 6 faces of the cube.
        // The cube is defined from (0,0,0) to (1,1,1).
        // Front face
        create_face_border({0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0});
        // Back face
        create_face_border({1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1});
        // Right face
        create_face_border({1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0});
        // Left face
        create_face_border({0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1});
        // Top face
        create_face_border({0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1});
        // Bottom face
        create_face_border({0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0});

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