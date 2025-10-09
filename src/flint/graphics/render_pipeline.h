#pragma once

#include <webgpu/webgpu.h>

namespace flint::graphics {

/**
 * @brief Represents a WebGPU render pipeline and its associated resources.
 *
 * This class is a resource-owning wrapper around the WGPURenderPipeline,
 * WGPUBindGroupLayout, and WGPUBindGroup handles. It is designed to be

 * constructed by the RenderPipelineBuilder and is move-only.
 */
class RenderPipeline {
public:
    /**
     * @brief Default constructor. Creates an empty, invalid pipeline.
     */
    RenderPipeline() = default;

    /**
     * @brief Constructs a new RenderPipeline.
     * @param pipeline The WebGPU render pipeline handle.
     * @param bind_group_layout The WebGPU bind group layout handle.
     * @param bind_group The WebGPU bind group handle.
     */
    RenderPipeline(WGPURenderPipeline pipeline, WGPUBindGroupLayout bind_group_layout, WGPUBindGroup bind_group);

    // Make the class move-only to ensure proper resource ownership.
    ~RenderPipeline();
    RenderPipeline(const RenderPipeline &) = delete;
    auto operator=(const RenderPipeline &) -> RenderPipeline & = delete;
    RenderPipeline(RenderPipeline &&other) noexcept;
    auto operator=(RenderPipeline &&other) noexcept -> RenderPipeline &;

    /**
     * @brief Gets the underlying WebGPU render pipeline handle.
     */
    [[nodiscard]] auto get_pipeline() const -> WGPURenderPipeline;

    /**
     * @brief Gets the underlying WebGPU bind group layout handle.
     */
    [[nodiscard]] auto get_bind_group_layout() const -> WGPUBindGroupLayout;

    /**
     * @brief Gets the underlying WebGPU bind group handle.
     */
    [[nodiscard]] auto get_bind_group() const -> WGPUBindGroup;

private:
    void cleanup();

    WGPURenderPipeline pipeline_ = nullptr;
    WGPUBindGroupLayout bind_group_layout_ = nullptr;
    WGPUBindGroup bind_group_ = nullptr;
};

} // namespace flint::graphics