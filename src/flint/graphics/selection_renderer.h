#pragma once

#include <webgpu/webgpu.h>
#include <vector>

#include "../camera.h"
#include "render_pipeline.h"
#include "../vertex.h"

namespace flint::graphics
{

    class SelectionRenderer
    {
    public:
        SelectionRenderer();
        ~SelectionRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat);
        void render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera);
        void cleanup();

    private:
        void createCubeMesh(WGPUDevice device);

        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_uniformBuffer = nullptr;
        CameraUniform m_cameraUniform;

        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        uint32_t m_indexCount = 0;
    };

} // namespace flint::graphics