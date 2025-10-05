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
    WGPUPrimitiveTopology topology,
        bool useTexture,
    bool useModelMatrix,
        bool depthWriteEnabled,
    WGPUCompareFunction depthCompare);

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
    WGPUBuffer cameraUniformBuffer,
    WGPUBuffer modelUniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler,
    bool useTexture,
    bool useModelMatrix);

    WGPURenderPassEncoder begin_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView,
        WGPUTextureView depthTextureView);

} // namespace flint::init