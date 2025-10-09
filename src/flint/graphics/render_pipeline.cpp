#include "render_pipeline.h"
#include <utility> // For std::swap

namespace flint::graphics {

RenderPipeline::RenderPipeline(WGPURenderPipeline pipeline, WGPUBindGroupLayout bind_group_layout, WGPUBindGroup bind_group)
    : m_pipeline(pipeline), m_bindGroupLayout(bind_group_layout), m_bindGroup(bind_group) {
}

RenderPipeline::~RenderPipeline() {
    cleanup();
}

RenderPipeline::RenderPipeline(RenderPipeline &&other) noexcept
    : m_pipeline(other.m_pipeline), m_bindGroupLayout(other.m_bindGroupLayout), m_bindGroup(other.m_bindGroup) {
    other.m_pipeline = nullptr;
    other.m_bindGroupLayout = nullptr;
    other.m_bindGroup = nullptr;
}

auto RenderPipeline::operator=(RenderPipeline &&other) noexcept -> RenderPipeline & {
    if (this != &other) {
        cleanup();
        m_pipeline = other.m_pipeline;
        m_bindGroupLayout = other.m_bindGroupLayout;
        m_bindGroup = other.m_bindGroup;
        other.m_pipeline = nullptr;
        other.m_bindGroupLayout = nullptr;
        other.m_bindGroup = nullptr;
    }
    return *this;
}

void RenderPipeline::cleanup() {
    if (m_bindGroup) {
        wgpuBindGroupRelease(m_bindGroup);
        m_bindGroup = nullptr;
    }
    if (m_bindGroupLayout) {
        wgpuBindGroupLayoutRelease(m_bindGroupLayout);
        m_bindGroupLayout = nullptr;
    }
    if (m_pipeline) {
        wgpuRenderPipelineRelease(m_pipeline);
        m_pipeline = nullptr;
    }
}

auto RenderPipeline::get_pipeline() const -> WGPURenderPipeline {
    return m_pipeline;
}

auto RenderPipeline::get_bind_group_layout() const -> WGPUBindGroupLayout {
    return m_bindGroupLayout;
}

auto RenderPipeline::get_bind_group() const -> WGPUBindGroup {
    return m_bindGroup;
}

} // namespace flint::graphics