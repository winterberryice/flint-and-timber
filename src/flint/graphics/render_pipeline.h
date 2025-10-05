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
            WGPUBuffer modelUniformBuffer, // Can be nullptr
            WGPUTextureView textureView,
            WGPUSampler sampler,
            WGPUPrimitiveTopology topology,
            bool useTexture,
            bool depthWriteEnabled,
            WGPUCompareFunction depthCompare,
            int32_t depthBias = 0,
            float depthBiasSlopeScale = 0.0f);

        void cleanup();

        WGPURenderPipeline getPipeline() const;
        WGPUBindGroup getBindGroup() const;

    private:
        WGPURenderPipeline m_pipeline = nullptr;
        WGPUBindGroupLayout m_bindGroupLayout = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;
    };

} // namespace flint::graphics