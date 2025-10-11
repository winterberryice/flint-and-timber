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

} // namespace flint::init