#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        WGPUBindGroupLayout *pBindGroupLayout,
        bool useTexture,
        bool useModel,
        bool depthWriteEnabled,
        WGPUCompareFunction depthCompare,
        bool useBlending,
        bool useCulling,
        WGPUPrimitiveTopology primitiveTopology = WGPUPrimitiveTopology_TriangleList,
        bool isUi = false);

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
        WGPUBuffer cameraUniformBuffer,
        WGPUBuffer modelUniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler,
        bool useTexture,
        bool useModel);

    WGPURenderPassEncoder begin_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView,
        WGPUTextureView depthTextureView);

} // namespace flint::init