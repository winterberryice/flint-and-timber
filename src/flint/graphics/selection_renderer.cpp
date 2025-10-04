#include "selection_renderer.h"
#include "../chunk.h"
#include "../vertex.h"
#include <vector>
#include <iostream>

namespace flint::graphics
{

    namespace
    {
        // Define the thickness of the selection outline.
        const float LINE_THICKNESS = 0.02f;
        // Define the gray color for the outline.
        const glm::vec3 OUTLINE_COLOR = {0.4f, 0.4f, 0.4f};

        // Helper to add vertices for a single quad.
        void add_quad(
            std::vector<flint::Vertex> &vertices,
            std::vector<uint16_t> &indices,
            const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3, const glm::vec3 &v4)
        {
            uint16_t base_index = static_cast<uint16_t>(vertices.size());
            vertices.push_back({v1, OUTLINE_COLOR, {0, 0}});
            vertices.push_back({v2, OUTLINE_COLOR, {0, 0}});
            vertices.push_back({v3, OUTLINE_COLOR, {0, 0}});
            vertices.push_back({v4, OUTLINE_COLOR, {0, 0}});
            indices.push_back(base_index);
            indices.push_back(base_index + 1);
            indices.push_back(base_index + 2);
            indices.push_back(base_index);
            indices.push_back(base_index + 2);
            indices.push_back(base_index + 3);
        }
    }

    SelectionRenderer::SelectionRenderer() = default;

    SelectionRenderer::~SelectionRenderer()
    {
        cleanup();
    }

    void SelectionRenderer::init(WGPUDevice device)
    {
        m_device = device;
    }

    void SelectionRenderer::cleanup()
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
        m_vertices.clear();
        m_indices.clear();
    }

    void SelectionRenderer::clear_mesh()
    {
        if (m_lastResult.has_value())
        {
            m_lastResult.reset();
            m_vertices.clear();
            m_indices.clear();
            create_buffers();
        }
    }

    void SelectionRenderer::update_mesh(const RaycastResult &result, const Chunk &chunk)
    {
        if (m_lastResult.has_value() &&
            m_lastResult->block_position == result.block_position &&
            m_lastResult->face == result.face)
        {
            return;
        }

        // First, check if the entire face is occluded. If so, don't draw anything.
        glm::ivec3 face_neighbor_pos = result.block_position;
        switch (result.face)
        {
        case BlockFace::Top:
            face_neighbor_pos.y += 1;
            break;
        case BlockFace::Bottom:
            face_neighbor_pos.y -= 1;
            break;
        case BlockFace::Left:
            face_neighbor_pos.x -= 1;
            break;
        case BlockFace::Right:
            face_neighbor_pos.x += 1;
            break;
        case BlockFace::Front:
            face_neighbor_pos.z -= 1;
            break;
        case BlockFace::Back:
            face_neighbor_pos.z += 1;
            break;
        }

        if (chunk.is_solid(face_neighbor_pos.x, face_neighbor_pos.y, face_neighbor_pos.z))
        {
            clear_mesh();
            m_lastResult = result; // Update to prevent re-checking
            return;
        }

        m_lastResult = result;
        m_vertices.clear();
        m_indices.clear();

        glm::vec3 p = glm::vec3(result.block_position);
        glm::ivec3 bp = result.block_position;
        float T = LINE_THICKNESS;

        // Define the 8 vertices of the cube
        glm::vec3 v000 = p;
        glm::vec3 v100 = p + glm::vec3(1, 0, 0);
        glm::vec3 v010 = p + glm::vec3(0, 1, 0);
        glm::vec3 v110 = p + glm::vec3(1, 1, 0);
        glm::vec3 v001 = p + glm::vec3(0, 0, 1);
        glm::vec3 v101 = p + glm::vec3(1, 0, 1);
        glm::vec3 v011 = p + glm::vec3(0, 1, 1);
        glm::vec3 v111 = p + glm::vec3(1, 1, 1);

        // Generate quads for the border, with culling for each edge.
        switch (result.face)
        {
        case BlockFace::Front: // -Z face
            if (!chunk.is_solid(bp.x, bp.y - 1, bp.z))
                add_quad(m_vertices, m_indices, v000, v100, v100 + glm::vec3(0, T, 0), v000 + glm::vec3(0, T, 0)); // Bottom
            if (!chunk.is_solid(bp.x, bp.y + 1, bp.z))
                add_quad(m_vertices, m_indices, v010 - glm::vec3(0, T, 0), v110 - glm::vec3(0, T, 0), v110, v010); // Top
            if (!chunk.is_solid(bp.x - 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v000, v010, v010 + glm::vec3(T, 0, 0), v000 + glm::vec3(T, 0, 0)); // Left
            if (!chunk.is_solid(bp.x + 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v100 - glm::vec3(T, 0, 0), v110 - glm::vec3(T, 0, 0), v110, v100); // Right
            break;
        case BlockFace::Back: // +Z face
            if (!chunk.is_solid(bp.x, bp.y - 1, bp.z))
                add_quad(m_vertices, m_indices, v001, v101, v101 + glm::vec3(0, T, 0), v001 + glm::vec3(0, T, 0)); // Bottom
            if (!chunk.is_solid(bp.x, bp.y + 1, bp.z))
                add_quad(m_vertices, m_indices, v011 - glm::vec3(0, T, 0), v111 - glm::vec3(0, T, 0), v111, v011); // Top
            if (!chunk.is_solid(bp.x - 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v001, v011, v011 + glm::vec3(T, 0, 0), v001 + glm::vec3(T, 0, 0)); // Left
            if (!chunk.is_solid(bp.x + 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v101 - glm::vec3(T, 0, 0), v111 - glm::vec3(T, 0, 0), v111, v101); // Right
            break;
        case BlockFace::Right: // +X face
            if (!chunk.is_solid(bp.x, bp.y - 1, bp.z))
                add_quad(m_vertices, m_indices, v100, v101, v101 + glm::vec3(0, T, 0), v100 + glm::vec3(0, T, 0)); // Bottom
            if (!chunk.is_solid(bp.x, bp.y + 1, bp.z))
                add_quad(m_vertices, m_indices, v110 - glm::vec3(0, T, 0), v111 - glm::vec3(0, T, 0), v111, v110); // Top
            if (!chunk.is_solid(bp.x, bp.y, bp.z - 1))
                add_quad(m_vertices, m_indices, v100, v110, v110 + glm::vec3(0, 0, T), v100 + glm::vec3(0, 0, T)); // Front
            if (!chunk.is_solid(bp.x, bp.y, bp.z + 1))
                add_quad(m_vertices, m_indices, v101, v111, v111 - glm::vec3(0, 0, T), v101 - glm::vec3(0, 0, T)); // Back
            break;
        case BlockFace::Left: // -X face
            if (!chunk.is_solid(bp.x, bp.y - 1, bp.z))
                add_quad(m_vertices, m_indices, v000, v001, v001 + glm::vec3(0, T, 0), v000 + glm::vec3(0, T, 0)); // Bottom
            if (!chunk.is_solid(bp.x, bp.y + 1, bp.z))
                add_quad(m_vertices, m_indices, v010 - glm::vec3(0, T, 0), v011 - glm::vec3(0, T, 0), v011, v010); // Top
            if (!chunk.is_solid(bp.x, bp.y, bp.z - 1))
                add_quad(m_vertices, m_indices, v000, v010, v010 + glm::vec3(0, 0, T), v000 + glm::vec3(0, 0, T)); // Front
            if (!chunk.is_solid(bp.x, bp.y, bp.z + 1))
                add_quad(m_vertices, m_indices, v001, v011, v011 - glm::vec3(0, 0, T), v001 - glm::vec3(0, 0, T)); // Back
            break;
        case BlockFace::Top: // +Y face
            if (!chunk.is_solid(bp.x, bp.y, bp.z - 1))
                add_quad(m_vertices, m_indices, v010, v110, v110 + glm::vec3(0, 0, T), v010 + glm::vec3(0, 0, T)); // Front
            if (!chunk.is_solid(bp.x, bp.y, bp.z + 1))
                add_quad(m_vertices, m_indices, v011, v111, v111 - glm::vec3(0, 0, T), v011 - glm::vec3(0, 0, T)); // Back
            if (!chunk.is_solid(bp.x - 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v010, v011, v011 + glm::vec3(T, 0, 0), v010 + glm::vec3(T, 0, 0)); // Left
            if (!chunk.is_solid(bp.x + 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v110, v111, v111 - glm::vec3(T, 0, 0), v110 - glm::vec3(T, 0, 0)); // Right
            break;
        case BlockFace::Bottom: // -Y face
            if (!chunk.is_solid(bp.x, bp.y, bp.z - 1))
                add_quad(m_vertices, m_indices, v000, v100, v100 + glm::vec3(0, 0, T), v000 + glm::vec3(0, 0, T)); // Front
            if (!chunk.is_solid(bp.x, bp.y, bp.z + 1))
                add_quad(m_vertices, m_indices, v001, v101, v101 - glm::vec3(0, 0, T), v001 - glm::vec3(0, 0, T)); // Back
            if (!chunk.is_solid(bp.x - 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v000, v001, v001 + glm::vec3(T, 0, 0), v000 + glm::vec3(T, 0, 0)); // Left
            if (!chunk.is_solid(bp.x + 1, bp.y, bp.z))
                add_quad(m_vertices, m_indices, v100, v101, v101 - glm::vec3(T, 0, 0), v100 - glm::vec3(T, 0, 0)); // Right
            break;
        }

        create_buffers();
    }

    void SelectionRenderer::create_buffers()
    {
        // Clean up old buffers first
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

        if (m_vertices.empty() || !m_device)
        {
            return;
        }

        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.size = m_vertices.size() * sizeof(flint::Vertex);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.mappedAtCreation = false;
        m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, m_vertices.data(), vertexBufferDesc.size);

        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.size = m_indices.size() * sizeof(uint16_t);
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        indexBufferDesc.mappedAtCreation = false;
        m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, m_indices.data(), indexBufferDesc.size);

        m_indexCount = static_cast<uint32_t>(m_indices.size());
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass) const
    {
        if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0)
        {
            return;
        }
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
    }

} // namespace flint::graphics