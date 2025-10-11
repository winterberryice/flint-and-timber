#pragma once

#include <webgpu/webgpu.h>

namespace flint::graphics
{

    class CrosshairMesh
    {
    public:
        CrosshairMesh() = default;
        ~CrosshairMesh() = default;

        void generate(WGPUDevice device, float aspectRatio);
        void render(WGPURenderPassEncoder renderPass) const;
        void cleanup();
        void updateAspectRatio(float aspectRatio);

    private:
        WGPUDevice m_device = nullptr;
        WGPUBuffer m_vertexBuffer = nullptr;
        uint32_t m_vertexCount = 0;
    };

} // namespace flint::graphics