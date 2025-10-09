#include "render_pipeline.h"
#include <utility> // For std::swap

namespace flint::graphics {

RenderPipeline::RenderPipeline(WGPURenderPipeline pipeline, WGPUBindGroupLayout bind_group_layout, WGPUBindGroup bind_group)
    : pipeline_(pipeline), bind_group_layout_(bind_group_layout), bind_group_(bind_group) {
}

RenderPipeline::~RenderPipeline() {
    cleanup();
}

RenderPipeline::RenderPipeline(RenderPipeline &&other) noexcept
    : pipeline_(other.pipeline_), bind_group_layout_(other.bind_group_layout_), bind_group_(other.bind_group_) {
    other.pipeline_ = nullptr;
    other.bind_group_layout_ = nullptr;
    other.bind_group_ = nullptr;
}

auto RenderPipeline::operator=(RenderPipeline &&other) noexcept -> RenderPipeline & {
    if (this != &other) {
        cleanup();
        pipeline_ = other.pipeline_;
        bind_group_layout_ = other.bind_group_layout_;
        bind_group_ = other.bind_group_;
        other.pipeline_ = nullptr;
        other.bind_group_layout_ = nullptr;
        other.bind_group_ = nullptr;
    }
    return *this;
}

void RenderPipeline::cleanup() {
    if (bind_group_) {
        wgpuBindGroupRelease(bind_group_);
        bind_group_ = nullptr;
    }
    if (bind_group_layout_) {
        wgpuBindGroupLayoutRelease(bind_group_layout_);
        bind_group_layout_ = nullptr;
    }
    if (pipeline_) {
        wgpuRenderPipelineRelease(pipeline_);
        pipeline_ = nullptr;
    }
}

auto RenderPipeline::get_pipeline() const -> WGPURenderPipeline {
    return pipeline_;
}

auto RenderPipeline::get_bind_group_layout() const -> WGPUBindGroupLayout {
    return bind_group_layout_;
}

auto RenderPipeline::get_bind_group() const -> WGPUBindGroup {
    return bind_group_;
}

} // namespace flint::graphics