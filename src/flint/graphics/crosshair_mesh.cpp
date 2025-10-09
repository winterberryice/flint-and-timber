#include "crosshair_mesh.h"

#include <vector>

#include "../init/buffer.h"

namespace flint::graphics
{

void CrosshairMesh::generate(WGPUDevice device, float aspectRatio)
    {
    const float length = 0.05f;    // The length of the lines as a proportion of screen height
    const float thickness = 0.005f; // The thickness of the lines as a proportion of screen height

    // Adjust for aspect ratio to make the lines appear equal in length
    const float h_len = length / aspectRatio; // Horizontal length adjusted for aspect ratio
    const float v_len = length;               // Vertical length

    const float h_thick = thickness;              // Horizontal thickness
    const float v_thick = thickness / aspectRatio; // Vertical thickness adjusted for aspect ratio

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

} // namespace flint::graphics