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
        float thickness = 0.02f;              // The thickness of the border.
        float epsilon = 0.001f;               // Small offset to prevent z-fighting.

        auto add_quad = [&](const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3)
        {
            uint32_t base_vertex = static_cast<uint32_t>(vertices.size());
            vertices.push_back({p0, color, {}});
            vertices.push_back({p1, color, {}});
            vertices.push_back({p2, color, {}});
            vertices.push_back({p3, color, {}});
            indices.push_back(base_vertex + 0);
            indices.push_back(base_vertex + 1);
            indices.push_back(base_vertex + 2);
            indices.push_back(base_vertex + 0);
            indices.push_back(base_vertex + 2);
            indices.push_back(base_vertex + 3);
        };

        // Since culling is disabled for the selection renderer, we don't need to worry
        // about the winding order of the faces. We can define them all consistently.

        // Front face (z = -epsilon)
        float z0 = -epsilon;
        add_quad({-epsilon, -epsilon, z0}, {1.0f + epsilon, -epsilon, z0}, {1.0f + epsilon, thickness, z0}, {-epsilon, thickness, z0});
        add_quad({-epsilon, 1.0f - thickness, z0}, {1.0f + epsilon, 1.0f - thickness, z0}, {1.0f + epsilon, 1.0f + epsilon, z0}, {-epsilon, 1.0f + epsilon, z0});
        add_quad({-epsilon, -epsilon, z0}, {thickness, -epsilon, z0}, {thickness, 1.0f + epsilon, z0}, {-epsilon, 1.0f + epsilon, z0});
        add_quad({1.0f - thickness, -epsilon, z0}, {1.0f + epsilon, -epsilon, z0}, {1.0f + epsilon, 1.0f + epsilon, z0}, {1.0f - thickness, 1.0f + epsilon, z0});

        // Back face (z = 1 + epsilon)
        float z1 = 1.0f + epsilon;
        add_quad({-epsilon, -epsilon, z1}, {1.0f + epsilon, -epsilon, z1}, {1.0f + epsilon, thickness, z1}, {-epsilon, thickness, z1});
        add_quad({-epsilon, 1.0f - thickness, z1}, {1.0f + epsilon, 1.0f - thickness, z1}, {1.0f + epsilon, 1.0f + epsilon, z1}, {-epsilon, 1.0f + epsilon, z1});
        add_quad({-epsilon, -epsilon, z1}, {thickness, -epsilon, z1}, {thickness, 1.0f + epsilon, z1}, {-epsilon, 1.0f + epsilon, z1});
        add_quad({1.0f - thickness, -epsilon, z1}, {1.0f + epsilon, -epsilon, z1}, {1.0f + epsilon, 1.0f + epsilon, z1}, {1.0f - thickness, 1.0f + epsilon, z1});

        // Left face (x = -epsilon)
        float x0 = -epsilon;
        add_quad({x0, -epsilon, -epsilon}, {x0, -epsilon, 1.0f + epsilon}, {x0, thickness, 1.0f + epsilon}, {x0, thickness, -epsilon});
        add_quad({x0, 1.0f - thickness, -epsilon}, {x0, 1.0f - thickness, 1.0f + epsilon}, {x0, 1.0f + epsilon, 1.0f + epsilon}, {x0, 1.0f + epsilon, -epsilon});
        add_quad({x0, -epsilon, -epsilon}, {x0, 1.0f + epsilon, -epsilon}, {x0, 1.0f + epsilon, thickness}, {x0, -epsilon, thickness});
        add_quad({x0, -epsilon, 1.0f - thickness}, {x0, 1.0f + epsilon, 1.0f - thickness}, {x0, 1.0f + epsilon, 1.0f + epsilon}, {x0, -epsilon, 1.0f + epsilon});

        // Right face (x = 1 + epsilon)
        float x1 = 1.0f + epsilon;
        add_quad({x1, -epsilon, -epsilon}, {x1, -epsilon, 1.0f + epsilon}, {x1, thickness, 1.0f + epsilon}, {x1, thickness, -epsilon});
        add_quad({x1, 1.0f - thickness, -epsilon}, {x1, 1.0f - thickness, 1.0f + epsilon}, {x1, 1.0f + epsilon, 1.0f + epsilon}, {x1, 1.0f + epsilon, -epsilon});
        add_quad({x1, -epsilon, -epsilon}, {x1, 1.0f + epsilon, -epsilon}, {x1, 1.0f + epsilon, thickness}, {x1, -epsilon, thickness});
        add_quad({x1, -epsilon, 1.0f - thickness}, {x1, 1.0f + epsilon, 1.0f - thickness}, {x1, 1.0f + epsilon, 1.0f + epsilon}, {x1, -epsilon, 1.0f + epsilon});

        // Bottom face (y = -epsilon)
        float y0 = -epsilon;
        add_quad({-epsilon, y0, -epsilon}, {-epsilon, y0, 1.0f + epsilon}, {thickness, y0, 1.0f + epsilon}, {thickness, y0, -epsilon});
        add_quad({1.0f - thickness, y0, -epsilon}, {1.0f - thickness, y0, 1.0f + epsilon}, {1.0f + epsilon, y0, 1.0f + epsilon}, {1.0f + epsilon, y0, -epsilon});
        add_quad({-epsilon, y0, -epsilon}, {1.0f + epsilon, y0, -epsilon}, {1.0f + epsilon, y0, thickness}, {-epsilon, y0, thickness});
        add_quad({-epsilon, y0, 1.0f - thickness}, {1.0f + epsilon, y0, 1.0f - thickness}, {1.0f + epsilon, y0, 1.0f + epsilon}, {-epsilon, y0, 1.0f + epsilon});

        // Top face (y = 1 + epsilon)
        float y1 = 1.0f + epsilon;
        add_quad({-epsilon, y1, -epsilon}, {-epsilon, y1, 1.0f + epsilon}, {thickness, y1, 1.0f + epsilon}, {thickness, y1, -epsilon});
        add_quad({1.0f - thickness, y1, -epsilon}, {1.0f - thickness, y1, 1.0f + epsilon}, {1.0f + epsilon, y1, 1.0f + epsilon}, {1.0f + epsilon, y1, -epsilon});
        add_quad({-epsilon, y1, -epsilon}, {1.0f + epsilon, y1, -epsilon}, {1.0f + epsilon, y1, thickness}, {-epsilon, y1, thickness});
        add_quad({-epsilon, y1, 1.0f - thickness}, {1.0f + epsilon, y1, 1.0f - thickness}, {1.0f + epsilon, y1, 1.0f + epsilon}, {-epsilon, y1, 1.0f + epsilon});

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