#include "selection_renderer.hpp"
#include "../vertex.h"
#include "glm/gtc/type_ptr.hpp"
#include <vector>

namespace flint
{
    namespace graphics
    {
        SelectionRenderer::~SelectionRenderer()
        {
            cleanup();
        }

        void SelectionRenderer::create_buffers(WGPUDevice device)
        {
            m_device = device;
        }

        void SelectionRenderer::update(const std::optional<glm::ivec3> &selected_block)
        {
            if (selected_block == m_lastSelectedBlock)
            {
                return;
            }

            m_lastSelectedBlock = selected_block;

            cleanup();

            if (!selected_block.has_value())
            {
                m_visible = false;
                return;
            }

            m_visible = true;

            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;

            auto add_quad = [&](const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3, const glm::vec3 &v4, const glm::vec3 &color)
            {
                uint16_t base_index = vertices.size();
                // We provide a color instead of a normal, and dummy UVs.
                vertices.push_back({v1, color, {0, 0}});
                vertices.push_back({v2, color, {0, 0}});
                vertices.push_back({v3, color, {0, 0}});
                vertices.push_back({v4, color, {0, 0}});
                indices.push_back(base_index + 0);
                indices.push_back(base_index + 1);
                indices.push_back(base_index + 2);
                indices.push_back(base_index + 0);
                indices.push_back(base_index + 2);
                indices.push_back(base_index + 3);
            };

            float x = selected_block->x;
            float y = selected_block->y;
            float z = selected_block->z;

            float t = 0.02f;
            float e = 0.002f;
            glm::vec3 color(1.0f, 1.0f, 1.0f); // White color for the outline

            // Top face (+Y)
            {
                float y_ = y + 1.0f + e;
                glm::vec3 v1(x, y_, z + 1), v2(x + 1, y_, z + 1), v3(x + 1, y_, z), v4(x, y_, z);
                glm::vec3 v1_in(x + t, y_, z + 1 - t), v2_in(x + 1 - t, y_, z + 1 - t), v3_in(x + 1 - t, y_, z + t), v4_in(x + t, y_, z + t);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            // Bottom face (-Y)
            {
                float y_ = y - e;
                glm::vec3 v1(x, y_, z), v2(x + 1, y_, z), v3(x + 1, y_, z + 1), v4(x, y_, z + 1);
                glm::vec3 v1_in(x + t, y_, z + t), v2_in(x + 1 - t, y_, z + t), v3_in(x + 1 - t, y_, z + 1 - t), v4_in(x + t, y_, z + 1 - t);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            // Right face (+X)
            {
                float x_ = x + 1.0f + e;
                glm::vec3 v1(x_, y, z), v2(x_, y + 1, z), v3(x_, y + 1, z + 1), v4(x_, y, z + 1);
                glm::vec3 v1_in(x_, y + t, z + t), v2_in(x_, y + 1 - t, z + t), v3_in(x_, y + 1 - t, z + 1 - t), v4_in(x_, y + t, z + 1 - t);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            // Left face (-X)
            {
                float x_ = x - e;
                glm::vec3 v1(x_, y, z + 1), v2(x_, y + 1, z + 1), v3(x_, y + 1, z), v4(x_, y, z);
                glm::vec3 v1_in(x_, y + t, z + 1 - t), v2_in(x_, y + 1 - t, z + 1 - t), v3_in(x_, y + 1 - t, z + t), v4_in(x_, y + t, z + t);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            // Front face (+Z)
            {
                float z_ = z + 1.0f + e;
                glm::vec3 v1(x + 1, y, z_), v2(x + 1, y + 1, z_), v3(x, y + 1, z_), v4(x, y, z_);
                glm::vec3 v1_in(x + 1 - t, y + t, z_), v2_in(x + 1 - t, y + 1 - t, z_), v3_in(x + t, y + 1 - t, z_), v4_in(x + t, y + t, z_);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            // Back face (-Z)
            {
                float z_ = z - e;
                glm::vec3 v1(x, y, z_), v2(x, y + 1, z_), v3(x + 1, y + 1, z_), v4(x + 1, y, z_);
                glm::vec3 v1_in(x + t, y + t, z_), v2_in(x + t, y + 1 - t, z_), v3_in(x + 1 - t, y + 1 - t, z_), v4_in(x + 1 - t, y + t, z_);
                add_quad(v1, v2, v2_in, v1_in, color);
                add_quad(v2, v3, v3_in, v2_in, color);
                add_quad(v3, v4, v4_in, v3_in, color);
                add_quad(v4, v1, v1_in, v4_in, color);
            }

            m_indexCount = indices.size();

            WGPUBufferDescriptor vertexBufferDesc;
            vertexBufferDesc.size = vertices.size() * sizeof(Vertex);
            vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            vertexBufferDesc.mappedAtCreation = false;
            m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

            WGPUBufferDescriptor indexBufferDesc;
            indexBufferDesc.size = indices.size() * sizeof(uint16_t);
            indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            indexBufferDesc.mappedAtCreation = false;
            m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
            wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, indices.data(), indexBufferDesc.size);
        }

        void SelectionRenderer::draw(WGPURenderPassEncoder renderPass) const
        {
            if (!m_visible || m_vertexBuffer == nullptr || m_indexBuffer == nullptr)
            {
                return;
            }

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
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
        }
    } // namespace graphics
} // namespace flint