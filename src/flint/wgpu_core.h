#pragma once

#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>

namespace flint
{

    void init_wgpu(
        const uint32_t width,
        const uint32_t height,
        SDL_Window *window,
        WGPUInstance &out_instance,
        WGPUSurface &out_surface,
        WGPUTextureFormat &out_surface_format,
        WGPUAdapter &out_adapter,
        WGPUDevice &out_device,
        WGPUQueue &out_queue //
    );

} // namespace flint