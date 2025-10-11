#include "crosshair_mesh.h"

#include <vector>

#include "../init/buffer.h"

namespace flint::graphics
{

void CrosshairMesh::generate(WGPUDevice device, int width, int height)
    {
    m_device = device;

    const float crosshair_len_px = 20.0f;
    const float crosshair_thickness_px = 2.0f;

    float h_len = crosshair_len_px / width;
    float v_len = crosshair_len_px / height;

    float h_thick = crosshair_thickness_px / height;
    float v_thick = crosshair_thickness_px / width;

        std::vector<float> vertices = {
        // Horizontal bar (two triangles)
        -h_len / 2, h_thick / 2,
        -h_len / 2, -h_thick / 2,
        h_len / 2, -h_thick / 2,

        h_len / 2, -h_thick / 2,
        h_len / 2, h_thick / 2,
        -h_len / 2, h_thick / 2,

        // Vertical bar (two triangles)
        -v_thick / 2, v_len / 2,
        -v_thick / 2, -v_len / 2,
        v_thick / 2, -v_len / 2,

        v_thick / 2, -v_len / 2,
        v_thick / 2, v_len / 2,
        -v_thick / 2, v_len / 2,
        };

        m_vertexCount = static_cast<uint32_t>(vertices.size() / 2);
        m_vertexBuffer = init::create_vertex_buffer(device, "Crosshair VB", vertices.data(), vertices.size() * sizeof(float));
    }

    void CrosshairMesh::render(WGPURenderPassEncoder renderPass) const
    {
        if (m_vertexBuffer)
        {
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDraw(renderPass, m_vertexCount, 1, 0, 0);
        }
    }

    void CrosshairMesh::cleanup()
    {
        if (m_vertexBuffer)
        {
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        m_vertexCount = 0;
    }

    void CrosshairMesh::onResize(int width, int height) {
        if (m_vertexBuffer)
        {
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }

        const float crosshair_len_px = 20.0f;
        const float crosshair_thickness_px = 2.0f;

        float h_len = crosshair_len_px / width;
        float v_len = crosshair_len_px / height;

        float h_thick = crosshair_thickness_px / height;
        float v_thick = crosshair_thickness_px / width;

        std::vector<float> vertices = {
        // Horizontal bar (two triangles)
        -h_len / 2, h_thick / 2,
        -h_len / 2, -h_thick / 2,
        h_len / 2, -h_thick / 2,

        h_len / 2, -h_thick / 2,
        h_len / 2, h_thick / 2,
        -h_len / 2, h_thick / 2,

        // Vertical bar (two triangles)
        -v_thick / 2, v_len / 2,
        -v_thick / 2, -v_len / 2,
        v_thick / 2, -v_len / 2,

        v_thick / 2, -v_len / 2,
        v_thick / 2, v_len / 2,
        -v_thick / 2, v_len / 2,
        };

        m_vertexCount = static_cast<uint32_t>(vertices.size() / 2);
        m_vertexBuffer = init::create_vertex_buffer(m_device, "Crosshair VB", vertices.data(), vertices.size() * sizeof(float));
    }

} // namespace flint::graphics