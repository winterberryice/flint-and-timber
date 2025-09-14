#include "crosshair.h"
#include "../glm/gtc/matrix_transform.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

namespace flint {
namespace ui {

// Helper to read a file, needed for the shader.
// TODO: This is a temporary utility. A better file utility should be used.
std::string read_shader_file_content(const char* path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file) {
        std::string contents;
        file.seekg(0, std::ios::end);
        contents.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&contents[0], contents.size());
        file.close();
        return contents;
    }
    // TODO: Proper error handling.
    std::cerr << "Failed to read file: " << path << std::endl;
    return "";
}

Crosshair::Crosshair(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height) {
    // --- Vertex Buffer ---
    const float half_length = 10.0f;
    const float thickness = 1.0f;
    const std::vector<CrosshairVertex> vertices = {
        // Horizontal bar
        {{-half_length, -thickness}}, {{ half_length, -thickness}}, {{-half_length,  thickness}},
        {{ half_length, -thickness}}, {{ half_length,  thickness}}, {{-half_length,  thickness}},
        // Vertical bar
        {{-thickness, -half_length}}, {{ thickness, -half_length}}, {{-thickness,  half_length}},
        {{ thickness, -half_length}}, {{ thickness,  half_length}}, {{-thickness,  half_length}},
    };
    num_vertices = static_cast<uint32_t>(vertices.size());
    uint64_t vertex_buffer_size = num_vertices * sizeof(CrosshairVertex);

    WGPUBufferDescriptor vertex_buffer_desc = {};
    vertex_buffer_desc.label = "Crosshair Vertex Buffer";
    vertex_buffer_desc.size = vertex_buffer_size;
    vertex_buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    vertex_buffer_desc.mappedAtCreation = true; // So we can write to it immediately
    vertex_buffer = wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);
    // TODO: The original code used create_buffer_init. This is the manual equivalent.
    // Need to get the mapped range and copy data.
    void* mapping = wgpuBufferGetMappedRange(vertex_buffer, 0, vertex_buffer_size);
    memcpy(mapping, vertices.data(), vertex_buffer_size);
    wgpuBufferUnmap(vertex_buffer);

    // --- Projection Matrix and Uniform Buffer ---
    projection_matrix = glm::ortho(
        -(width / 2.0f), width / 2.0f,
        height / 2.0f, -(height / 2.0f),
        -1.0f, 1.0f
    );

    WGPUBufferDescriptor proj_buffer_desc = {};
    proj_buffer_desc.label = "Crosshair Projection Buffer";
    proj_buffer_desc.size = sizeof(glm::mat4);
    proj_buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    proj_buffer_desc.mappedAtCreation = false;
    projection_buffer = wgpuDeviceCreateBuffer(device, &proj_buffer_desc);
    // The queue is not available here, so initial data will be written in the first resize/update.

    // --- Bind Group Layout and Bind Group ---
    WGPUBindGroupLayoutEntry bgl_entry = {};
    bgl_entry.binding = 0;
    bgl_entry.visibility = WGPUShaderStage_Vertex;
    bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;

    WGPUBindGroupLayoutDescriptor bgl_desc = {};
    bgl_desc.label = "Crosshair Projection BGL";
    bgl_desc.entryCount = 1;
    bgl_desc.entries = &bgl_entry;
    projection_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bgl_desc);

    WGPUBindGroupEntry bg_entry = {};
    bg_entry.binding = 0;
    bg_entry.buffer = projection_buffer;
    bg_entry.size = sizeof(glm::mat4);

    WGPUBindGroupDescriptor bg_desc = {};
    bg_desc.label = "Crosshair Projection BG";
    bg_desc.layout = projection_bind_group_layout;
    bg_desc.entryCount = 1;
    bg_desc.entries = &bg_entry;
    projection_bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);

    // --- Shader and Pipeline ---
    std::string shader_code = read_shader_file_content("src_from_rust/crosshair_shader.wgsl");
    WGPUShaderModuleWGSLDescriptor shader_wgsl_desc = {};
    shader_wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_wgsl_desc.source = shader_code.c_str();
    WGPUShaderModuleDescriptor shader_desc = { .nextInChain = &shader_wgsl_desc.chain, .label = "Crosshair Shader" };
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    WGPUPipelineLayoutDescriptor layout_desc = { .label = "Crosshair Pipeline Layout", .bindGroupLayoutCount = 1, .bindGroupLayouts = &projection_bind_group_layout };
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPUVertexBufferLayout buffer_layout = {};
    buffer_layout.arrayStride = sizeof(CrosshairVertex);
    buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
    WGPUVertexAttribute vert_attr = { .format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0 };
    buffer_layout.attributeCount = 1;
    buffer_layout.attributes = &vert_attr;

    WGPUBlendState blend_state = { .color = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha }, .alpha = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha } };
    WGPUColorTargetState color_target = { .format = surface_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All };
    WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = "fs_main", .targetCount = 1, .targets = &color_target };

    WGPURenderPipelineDescriptor pipeline_desc = {
        .label = "Crosshair Render Pipeline",
        .layout = pipeline_layout,
        .vertex = { .module = shader_module, .entryPoint = "vs_main", .bufferCount = 1, .buffers = &buffer_layout },
        .fragment = &fragment_state,
        .primitive = { .topology = WGPUPrimitiveTopology_TriangleList, .frontFace = WGPUFrontFace_CCW },
    };
    render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuShaderModuleRelease(shader_module);

    // Write initial projection matrix data
    resize(width, height, wgpuDeviceGetQueue(device));
}

Crosshair::~Crosshair() {
    wgpuBufferRelease(vertex_buffer);
    wgpuBufferRelease(projection_buffer);
    wgpuBindGroupLayoutRelease(projection_bind_group_layout);
    wgpuBindGroupRelease(projection_bind_group);
    wgpuRenderPipelineRelease(render_pipeline);
}

void Crosshair::resize(uint32_t new_width, uint32_t new_height, WGPUQueue queue) {
    if (new_width > 0 && new_height > 0) {
        projection_matrix = glm::ortho(
            -(new_width / 2.0f), new_width / 2.0f,
            new_height / 2.0f, -(new_height / 2.0f),
            -1.0f, 1.0f
        );
        wgpuQueueWriteBuffer(queue, projection_buffer, 0, &projection_matrix, sizeof(glm::mat4));
    }
}

void Crosshair::draw(WGPURenderPassEncoder render_pass) {
    wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, projection_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderDraw(render_pass, num_vertices, 1, 0, 0);
}

} // namespace ui
} // namespace flint
