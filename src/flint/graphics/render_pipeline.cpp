#include "render_pipeline.h"

#include "../init/pipeline.h"

namespace flint::graphics
{

    RenderPipeline::RenderPipeline() = default;

    RenderPipeline::~RenderPipeline() = default;

    void RenderPipeline::init(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        WGPUBuffer uniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler)
    {
        m_pipeline = flint::init::create_render_pipeline(
            device,
            vertexShader,
            fragmentShader,
            surfaceFormat,
            depthTextureFormat,
            &m_bindGroupLayout);

        m_bindGroup = flint::init::create_bind_group(
            device,
            m_bindGroupLayout,
            uniformBuffer,
            textureView,
            sampler);
    }

    void RenderPipeline::cleanup()
    {
        if (m_bindGroup)
        {
            wgpuBindGroupRelease(m_bindGroup);
            m_bindGroup = nullptr;
        }
        if (m_bindGroupLayout)
        {
            wgpuBindGroupLayoutRelease(m_bindGroupLayout);
            m_bindGroupLayout = nullptr;
        }
        if (m_pipeline)
        {
            wgpuRenderPipelineRelease(m_pipeline);
            m_pipeline = nullptr;
        }
    }

    WGPURenderPipeline RenderPipeline::getPipeline() const
    {
        return m_pipeline;
    }

    WGPUBindGroup RenderPipeline::getBindGroup() const
    {
        return m_bindGroup;
    }

} // namespace flint::graphics