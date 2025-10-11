#pragma once

#include <webgpu/webgpu.h>

namespace flint::graphics
{

    class CrosshairMesh
    {
    public:
        CrosshairMesh() = default;
        ~CrosshairMesh() = default;

        void generate(WGPUDevice device, int width, int height);
        void render(WGPURenderPassEncoder renderPass) const;
        void cleanup();
        void onResize(int width, int height);

    private:
        WGPUDevice m_device = nullptr;
        WGPUBuffer m_vertexBuffer = nullptr;
        uint32_t m_vertexCount = 0;
    };

} // namespace flint::graphics