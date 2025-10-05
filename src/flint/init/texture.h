#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    void create_depth_texture(
        WGPUDevice device,
        uint32_t width,
        uint32_t height,
        WGPUTextureFormat format,
        WGPUTexture *pTexture,
        WGPUTextureView *pTextureView);

} // namespace flint::init