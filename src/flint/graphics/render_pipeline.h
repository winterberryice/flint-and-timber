#pragma once

#include <webgpu/webgpu.h>

namespace flint::graphics
{

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

        void cleanup();

        WGPURenderPipeline pipeline = nullptr;
        WGPUBindGroupLayout bindGroupLayout = nullptr;
        WGPUBindGroup bindGroup = nullptr;
    };

} // namespace flint::graphics