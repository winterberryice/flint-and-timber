#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUBindGroupLayout *pBindGroupLayout // Output parameter
    );

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
        WGPUBuffer uniformBuffer);

} // namespace flint::init