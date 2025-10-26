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

    void create_texture(
        WGPUDevice device,
        WGPUQueue queue,
        const void *data,
        uint32_t width,
        uint32_t height,
        WGPUTextureFormat format,
        WGPUTexture *pTexture,
        WGPUTextureView *pTextureView);

    WGPUSampler create_sampler(WGPUDevice device);

} // namespace flint::init