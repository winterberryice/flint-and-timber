#pragma once

#include <webgpu/webgpu.h>

#include "../camera.h"
#include "render_pipeline.h"
#include "selection_mesh.h"

namespace flint::graphics
{

    class SelectionRenderer
    {
    public:
        SelectionRenderer();
        ~SelectionRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat);
        void generateSelectionBox(WGPUDevice device);
        void render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera);
        void cleanup();

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_uniformBuffer = nullptr;
        CameraUniform m_cameraUniform;

        SelectionMesh m_selectionMesh;
    };

} // namespace flint::graphics