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
        WGPUBindGroupLayout *pBindGroupLayout);

    WGPURenderPipeline create_selection_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        WGPUBindGroupLayout bindGroupLayout); // Takes the main layout as input

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
        WGPUBuffer uniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler);

} // namespace flint::init