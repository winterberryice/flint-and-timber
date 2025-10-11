#pragma once

#include <webgpu/webgpu.h>

#include "crosshair_mesh.h"
#include "render_pipeline.h"

namespace flint::graphics
{

    class CrosshairRenderer
    {
    public:
        CrosshairRenderer();
        ~CrosshairRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, float aspectRatio);
        void render(WGPURenderPassEncoder renderPass);
        void cleanup();
        void updateAspectRatio(float aspectRatio);

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        RenderPipeline m_renderPipeline;
        CrosshairMesh m_crosshairMesh;
    };

} // namespace flint::graphics