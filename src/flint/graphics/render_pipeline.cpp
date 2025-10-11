#include "render_pipeline.h"

namespace flint::graphics
{
    RenderPipeline::RenderPipeline() = default;

    RenderPipeline::~RenderPipeline() = default;

    void RenderPipeline::cleanup()
    {
        if (bindGroup)
        {
            wgpuBindGroupRelease(bindGroup);
            bindGroup = nullptr;
        }
        if (bindGroupLayout)
        {
            wgpuBindGroupLayoutRelease(bindGroupLayout);
            bindGroupLayout = nullptr;
        }
        if (pipeline)
        {
            wgpuRenderPipelineRelease(pipeline);
            pipeline = nullptr;
        }
    }
} // namespace flint::graphics