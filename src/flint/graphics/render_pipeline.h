#pragma once

#include <webgpu/webgpu.h>

namespace flint::graphics
{

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

        void init(
            WGPUDevice device,
            WGPUShaderModule vertexShader,
            WGPUShaderModule fragmentShader,
            WGPUTextureFormat surfaceFormat,
            WGPUTextureFormat depthTextureFormat,
            WGPUBuffer cameraUniformBuffer,
            WGPUBuffer modelUniformBuffer,
            WGPUTextureView textureView,
            WGPUSampler sampler,
            bool useTexture,
            bool useModel,
            bool depthWriteEnabled,
        WGPUCompareFunction depthCompare,
        WGPUPrimitiveTopology topology,
        bool use_blending);

        void cleanup();

        WGPURenderPipeline getPipeline() const;
        WGPUBindGroup getBindGroup() const;

    private:
        WGPURenderPipeline m_pipeline = nullptr;
        WGPUBindGroupLayout m_bindGroupLayout = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;
    };

} // namespace flint::graphics