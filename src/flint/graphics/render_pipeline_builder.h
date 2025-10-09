#pragma once

#include "render_pipeline.h"
#include <webgpu/webgpu.h>
#include <vector>

namespace flint::graphics {

/**
 * @brief A builder for creating WGPURenderPipeline objects.
 *
 * This class provides a fluent interface for configuring and constructing a render pipeline,
 * simplifying the setup process and improving code readability.
 */
class RenderPipelineBuilder {
public:
    /**
     * @brief Constructs a new RenderPipelineBuilder.
     * @param device The WebGPU device.
     */
    explicit RenderPipelineBuilder(WGPUDevice device);

    /**
     * @brief Sets the vertex and fragment shaders for the pipeline.
     */
    auto with_shaders(WGPUShaderModule vertex_shader, WGPUShaderModule fragment_shader) -> RenderPipelineBuilder &;

    /**
     * @brief Sets the color format of the render target.
     */
    auto with_surface_format(WGPUTextureFormat format) -> RenderPipelineBuilder &;

    /**
     * @brief Sets the format of the depth texture.
     */
    auto with_depth_format(WGPUTextureFormat format) -> RenderPipelineBuilder &;

    /**
     * @brief Adds a uniform buffer for the camera.
     */
    auto with_camera_uniform(WGPUBuffer buffer) -> RenderPipelineBuilder &;

    /**
     * @brief Adds a uniform buffer for model transformations.
     */
    auto with_model_uniform(WGPUBuffer buffer) -> RenderPipelineBuilder &;

    /**
     * @brief Adds a texture and sampler to the pipeline.
     */
    auto with_texture(WGPUTextureView view, WGPUSampler sampler) -> RenderPipelineBuilder &;

    /**
     * @brief Sets the primitive topology for drawing.
     */
    auto with_primitive_topology(WGPUPrimitiveTopology topology) -> RenderPipelineBuilder &;

    /**
     * @brief Enables or disables writing to the depth buffer.
     */
    auto enable_depth_write(bool enabled) -> RenderPipelineBuilder &;

    /**
     * @brief Sets the comparison function for the depth test.
     */
    auto with_depth_compare(WGPUCompareFunction function) -> RenderPipelineBuilder &;

    /**
     * @brief Enables or disables alpha blending.
     */
    auto enable_blending(bool enabled) -> RenderPipelineBuilder &;

    /**
     * @brief Enables or disables back-face culling.
     */
    auto enable_culling(bool enabled) -> RenderPipelineBuilder &;

    /**
     * @brief Builds the render pipeline with the specified configuration.
     * @return A configured RenderPipeline object.
     */
    auto build() -> RenderPipeline;

private:
    WGPUDevice device_{};
    WGPUShaderModule vertex_shader_{};
    WGPUShaderModule fragment_shader_{};
    WGPUTextureFormat surface_format_ = WGPUTextureFormat_Undefined;
    WGPUTextureFormat depth_format_ = WGPUTextureFormat_Undefined;
    WGPUBuffer camera_uniform_{};
    WGPUBuffer model_uniform_{};
    WGPUTextureView texture_view_{};
    WGPUSampler sampler_{};
    WGPUPrimitiveTopology primitive_topology_ = WGPUPrimitiveTopology_TriangleList;
    bool depth_write_enabled_ = false;
    WGPUCompareFunction depth_compare_ = WGPUCompareFunction_Always;
    bool blending_enabled_ = false;
    bool culling_enabled_ = false;
};

} // namespace flint::graphics