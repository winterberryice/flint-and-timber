#include "render_pipeline_builder.h"
#include "../vertex.h" // For vertex buffer layout.
#include <iostream>
#include <vector>

namespace flint::graphics {

RenderPipelineBuilder::RenderPipelineBuilder(WGPUDevice device) : device_(device) {
}

auto RenderPipelineBuilder::with_shaders(WGPUShaderModule vertex_shader, WGPUShaderModule fragment_shader)
    -> RenderPipelineBuilder & {
    vertex_shader_ = vertex_shader;
    fragment_shader_ = fragment_shader;
    return *this;
}

auto RenderPipelineBuilder::with_surface_format(WGPUTextureFormat format) -> RenderPipelineBuilder & {
    surface_format_ = format;
    return *this;
}

auto RenderPipelineBuilder::with_depth_format(WGPUTextureFormat format) -> RenderPipelineBuilder & {
    depth_format_ = format;
    return *this;
}

auto RenderPipelineBuilder::with_camera_uniform(WGPUBuffer buffer) -> RenderPipelineBuilder & {
    camera_uniform_ = buffer;
    return *this;
}

auto RenderPipelineBuilder::with_model_uniform(WGPUBuffer buffer) -> RenderPipelineBuilder & {
    model_uniform_ = buffer;
    return *this;
}

auto RenderPipelineBuilder::with_texture(WGPUTextureView view, WGPUSampler sampler) -> RenderPipelineBuilder & {
    texture_view_ = view;
    sampler_ = sampler;
    return *this;
}

auto RenderPipelineBuilder::with_primitive_topology(WGPUPrimitiveTopology topology) -> RenderPipelineBuilder & {
    primitive_topology_ = topology;
    return *this;
}

auto RenderPipelineBuilder::enable_depth_write(bool enabled) -> RenderPipelineBuilder & {
    depth_write_enabled_ = enabled;
    return *this;
}

auto RenderPipelineBuilder::with_depth_compare(WGPUCompareFunction function) -> RenderPipelineBuilder & {
    depth_compare_ = function;
    return *this;
}

auto RenderPipelineBuilder::enable_blending(bool enabled) -> RenderPipelineBuilder & {
    blending_enabled_ = enabled;
    return *this;
}

auto RenderPipelineBuilder::enable_culling(bool enabled) -> RenderPipelineBuilder & {
    culling_enabled_ = enabled;
    return *this;
}

auto RenderPipelineBuilder::uses_vertex_buffer(bool enabled) -> RenderPipelineBuilder & {
    use_vertex_buffer_ = enabled;
    return *this;
}

auto RenderPipelineBuilder::build() -> RenderPipeline {
    // --- Bind Group Layout ---
    std::vector<WGPUBindGroupLayoutEntry> bg_layout_entries;
    std::vector<WGPUBindGroupEntry> bg_entries;
    uint32_t binding_index = 0;

    if (camera_uniform_) {
        bg_layout_entries.push_back(WGPUBindGroupLayoutEntry{
            .binding = binding_index,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = {.type = WGPUBufferBindingType_Uniform},
        });
        bg_entries.push_back(WGPUBindGroupEntry{.binding = binding_index, .buffer = camera_uniform_, .size = WGPU_WHOLE_SIZE});
        binding_index++;
    }
    if (model_uniform_) {
        bg_layout_entries.push_back(WGPUBindGroupLayoutEntry{
            .binding = binding_index,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = {.type = WGPUBufferBindingType_Uniform},
        });
        bg_entries.push_back(WGPUBindGroupEntry{.binding = binding_index, .buffer = model_uniform_, .size = WGPU_WHOLE_SIZE});
        binding_index++;
    }
    if (texture_view_) {
        bg_layout_entries.push_back(WGPUBindGroupLayoutEntry{
            .binding = binding_index,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {.sampleType = WGPUTextureSampleType_Float},
        });
        bg_entries.push_back(WGPUBindGroupEntry{.binding = binding_index, .textureView = texture_view_});
        binding_index++;
    }
    if (sampler_) {
        bg_layout_entries.push_back(WGPUBindGroupLayoutEntry{
            .binding = binding_index,
            .visibility = WGPUShaderStage_Fragment,
            .sampler = {.type = WGPUSamplerBindingType_Filtering},
        });
        bg_entries.push_back(WGPUBindGroupEntry{.binding = binding_index, .sampler = sampler_});
        binding_index++;
    }

    WGPUBindGroupLayoutDescriptor bg_layout_desc{
        .entryCount = static_cast<uint32_t>(bg_layout_entries.size()),
        .entries = bg_layout_entries.data(),
    };
    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(device_, &bg_layout_desc);

    // --- Bind Group ---
    WGPUBindGroup bind_group = nullptr;
    if (!bg_entries.empty()) {
        WGPUBindGroupDescriptor bg_desc{
            .layout = bind_group_layout,
            .entryCount = static_cast<uint32_t>(bg_entries.size()),
            .entries = bg_entries.data(),
        };
        bind_group = wgpuDeviceCreateBindGroup(device_, &bg_desc);
    }

    // --- Pipeline Layout ---
    WGPUPipelineLayoutDescriptor layout_desc{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &bind_group_layout,
    };
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device_, &layout_desc);

    // --- Shaders ---
    WGPUVertexState vertex_state{
        .module = vertex_shader_,
        .entryPoint = "vs_main",
    };

    WGPUVertexBufferLayout vb_layout;
    if (use_vertex_buffer_) {
        vb_layout = Vertex::getLayout();
        vertex_state.bufferCount = 1;
        vertex_state.buffers = &vb_layout;
    } else {
        vertex_state.bufferCount = 0;
        vertex_state.buffers = nullptr;
    }

    WGPUColorTargetState color_target{
        .nextInChain = nullptr,
        .format = surface_format_,
        .blend = nullptr,
        .writeMask = WGPUColorWriteMask_All,
    };

    WGPUBlendState blend_state; // Safely stack-allocated
    if (blending_enabled_) {
        blend_state = {
            .color = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha},
            .alpha = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One, .dstFactor = WGPUBlendFactor_Zero}
        };
        color_target.blend = &blend_state;
    }

    WGPUFragmentState fragment_state{
        .module = fragment_shader_,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = &color_target,
    };

    // --- Depth Stencil ---
    WGPUDepthStencilState depth_stencil_state{};
    if (depth_format_ != WGPUTextureFormat_Undefined) {
        depth_stencil_state.format = depth_format_;
        depth_stencil_state.depthWriteEnabled = depth_write_enabled_ ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depth_stencil_state.depthCompare = depth_compare_;
    }

    // --- Primitive State ---
    WGPUPrimitiveState primitive_state{
        .topology = primitive_topology_,
        .stripIndexFormat = WGPUIndexFormat_Undefined,
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = culling_enabled_ ? WGPUCullMode_Back : WGPUCullMode_None,
    };

    // --- Render Pipeline Descriptor ---
    WGPURenderPipelineDescriptor pipeline_desc{
        .layout = pipeline_layout,
        .vertex = vertex_state,
        .primitive = primitive_state,
        .depthStencil = (depth_format_ != WGPUTextureFormat_Undefined) ? &depth_stencil_state : nullptr,
        .multisample = {.count = 1, .mask = 0xFFFFFFFF},
        .fragment = &fragment_state,
    };

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device_, &pipeline_desc);

    wgpuPipelineLayoutRelease(pipeline_layout);

    return RenderPipeline(pipeline, bind_group_layout, bind_group);
}

} // namespace flint::graphics