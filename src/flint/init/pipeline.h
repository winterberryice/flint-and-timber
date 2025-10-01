#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

#include <vector>

    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat, // New parameter for depth texture
    const std::vector<WGPUBindGroupLayout> &bindGroupLayouts);


} // namespace flint::init