#include "crosshair_mesh.h"

#include <vector>

#include "../init/buffer.h"

namespace flint::graphics
{

    void CrosshairMesh::generate(WGPUDevice device)
    {
        // A simple crosshair mesh '+'
        const float size = 0.05f;
        std::vector<float> vertices = {
            // Horizontal line
            -size, 0.0f,
            size, 0.0f,

            // Vertical line
            0.0f, -size,
            0.0f, size,
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