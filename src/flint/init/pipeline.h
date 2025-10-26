#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPURenderPassEncoder begin_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView,
        WGPUTextureView depthTextureView);

    WGPURenderPassEncoder begin_overlay_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView);

    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule shaderModule,
        WGPUFragmentState *fragmentState,
        WGPUVertexBufferLayout *vertexBufferLayout,
        WGPUPrimitiveTopology primitiveTopology,
        WGPUDepthStencilState *depthStencilState);

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPURenderPipeline pipeline,
        uint32_t groupIndex,
        WGPUBindGroupEntry *entries,
        uint32_t entryCount);

} // namespace flint::init