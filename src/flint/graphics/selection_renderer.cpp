#include "selection_renderer.hpp"
#include "../selection_shader.wgsl.h"
#include "../cube_geometry.h"
#include "../vertex.h"
#include "../init/utils.h"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace flint::graphics {

// Helper to map BlockFace to neighbor offset
glm::ivec3 get_neighbor_offset(BlockFace face) {
    switch (face) {
        case BlockFace::PosX: return {1, 0, 0};
        case BlockFace::NegX: return {-1, 0, 0};
        case BlockFace::PosY: return {0, 1, 0};
        case BlockFace::NegY: return {0, -1, 0};
        case BlockFace::PosZ: return {0, 0, 1};
        case BlockFace::NegZ: return {0, 0, -1};
    }
    return {0, 0, 0}; // Should not happen
}

// Helper to map BlockFace to CubeGeometry::Face
CubeGeometry::Face to_cube_geometry_face(BlockFace face) {
    switch (face) {
        case BlockFace::PosX: return CubeGeometry::Face::Right;
        case BlockFace::NegX: return CubeGeometry::Face::Left;
        case BlockFace::PosY: return CubeGeometry::Face::Top;
        case BlockFace::NegY: return CubeGeometry::Face::Bottom;
        case BlockFace::PosZ: return CubeGeometry::Face::Back;
        case BlockFace::NegZ: return CubeGeometry::Face::Front;
    }
    return CubeGeometry::Face::Front;
}


SelectionRenderer::SelectionRenderer(WGPUDevice device, WGPUTextureFormat surface_format, const WGPUBindGroupLayout& camera_bind_group_layout)
    : device(device), surface_format(surface_format), camera_bind_group_layout(camera_bind_group_layout) {
    create_buffers();
    create_pipeline();
    create_bind_group();
}

SelectionRenderer::~SelectionRenderer() {
    wgpuBufferRelease(vertex_buffer);
    wgpuBufferRelease(index_buffer);
    wgpuRenderPipelineRelease(render_pipeline);
    wgpuBufferRelease(model_uniform_buffer);
    wgpuBindGroupRelease(model_bind_group);
    wgpuBindGroupLayoutRelease(model_bind_group_layout);
}

void SelectionRenderer::create_buffers() {
    const auto& vertices = CubeGeometry::getVertices();
    WGPUBufferDescriptor buffer_desc{};
    buffer_desc.label = init::makeStringView("Selection Vertex Buffer");
    buffer_desc.usage = WGPUBufferUsage_Vertex;
    buffer_desc.size = vertices.size() * sizeof(Vertex);
    buffer_desc.mappedAtCreation = true;
    vertex_buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);
    memcpy(wgpuBufferGetMappedRange(vertex_buffer, 0, buffer_desc.size), vertices.data(), buffer_desc.size);
    wgpuBufferUnmap(vertex_buffer);

    const auto& indices = CubeGeometry::getIndices();
    index_count = indices.size();
    WGPUBufferDescriptor index_buffer_desc{};
    index_buffer_desc.label = init::makeStringView("Selection Index Buffer");
    index_buffer_desc.usage = WGPUBufferUsage_Index;
    index_buffer_desc.size = indices.size() * sizeof(uint16_t);
    index_buffer_desc.mappedAtCreation = true;
    index_buffer = wgpuDeviceCreateBuffer(device, &index_buffer_desc);
    memcpy(wgpuBufferGetMappedRange(index_buffer, 0, index_buffer_desc.size), indices.data(), index_buffer_desc.size);
    wgpuBufferUnmap(index_buffer);
}


void SelectionRenderer::create_pipeline() {
    WGPUShaderSourceWGSL wgsl_desc{};
    wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl_desc.code = init::makeStringView(shaders::selection_shader_wgsl);

    WGPUShaderModuleDescriptor shader_desc{};
    shader_desc.nextInChain = &wgsl_desc.chain;
    shader_desc.label = init::makeStringView("Selection Shader");
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    WGPUBindGroupLayoutEntry model_bgl_entry{};
    model_bgl_entry.binding = 0;
    model_bgl_entry.visibility = WGPUShaderStage_Vertex;
    model_bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;

    WGPUBindGroupLayoutDescriptor model_bgl_desc{};
    model_bgl_desc.label = init::makeStringView("Selection Model BGL");
    model_bgl_desc.entryCount = 1;
    model_bgl_desc.entries = &model_bgl_entry;
    model_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &model_bgl_desc);

    std::vector<WGPUBindGroupLayout> bind_group_layouts = {camera_bind_group_layout, model_bind_group_layout};

    WGPUPipelineLayoutDescriptor layout_desc{};
    layout_desc.label = init::makeStringView("Selection Pipeline Layout");
    layout_desc.bindGroupLayoutCount = bind_group_layouts.size();
    layout_desc.bindGroupLayouts = bind_group_layouts.data();
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPUVertexState vertex_state{};
    vertex_state.module = shader_module;
    vertex_state.entryPoint = init::makeStringView("vs_main");
    WGPUVertexBufferLayout vertex_buffer_layout = Vertex::getLayout();
    vertex_state.bufferCount = 1;
    vertex_state.buffers = &vertex_buffer_layout;

    WGPUColorTargetState color_target{};
    color_target.format = surface_format;
    color_target.writeMask = WGPUColorWriteMask_All;
    WGPUBlendState blend_state{};
    blend_state.color.operation = WGPUBlendOperation_Add;
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.alpha.operation = WGPUBlendOperation_Add;
    blend_state.alpha.srcFactor = WGPUBlendFactor_One;
    blend_state.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    color_target.blend = &blend_state;

    WGPUFragmentState fragment_state{};
    fragment_state.module = shader_module;
    fragment_state.entryPoint = init::makeStringView("fs_main");
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    WGPUDepthStencilState depth_stencil_state{};
    depth_stencil_state.format = WGPUTextureFormat_Depth24Plus;
    depth_stencil_state.depthWriteEnabled = WGPUOptionalBool_False;
    depth_stencil_state.depthCompare = WGPUCompareFunction_LessEqual;
    depth_stencil_state.depthBias = -2;
    depth_stencil_state.depthBiasSlopeScale = -2.0f;

    WGPURenderPipelineDescriptor pipeline_desc{};
    pipeline_desc.label = init::makeStringView("Selection Render Pipeline");
    pipeline_desc.layout = pipeline_layout;
    pipeline_desc.vertex = vertex_state;
    pipeline_desc.fragment = &fragment_state;
    pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
    pipeline_desc.primitive.cullMode = WGPUCullMode_None;
    pipeline_desc.depthStencil = &depth_stencil_state;
    pipeline_desc.multisample.count = 1;

    render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuShaderModuleRelease(shader_module);
}

void SelectionRenderer::create_bind_group() {
    WGPUBufferDescriptor uniform_buffer_desc{};
    uniform_buffer_desc.label = init::makeStringView("Selection Model Uniform Buffer");
    uniform_buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    uniform_buffer_desc.size = sizeof(glm::mat4);
    model_uniform_buffer = wgpuDeviceCreateBuffer(device, &uniform_buffer_desc);

    WGPUBindGroupEntry bg_entry{};
    bg_entry.binding = 0;
    bg_entry.buffer = model_uniform_buffer;
    bg_entry.size = sizeof(glm::mat4);

    WGPUBindGroupDescriptor bg_desc{};
    bg_desc.label = init::makeStringView("Selection Model Bind Group");
    bg_desc.layout = model_bind_group_layout;
    bg_desc.entryCount = 1;
    bg_desc.entries = &bg_entry;
    model_bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);
}

void SelectionRenderer::draw(
    WGPURenderPassEncoder render_pass,
    WGPUQueue queue,
    const WGPUBindGroup& camera_bind_group,
    const Chunk& chunk,
    const std::optional<RaycastResult>& selection) {

    if (!selection.has_value()) {
        return;
    }

    const auto& result = selection.value();
    glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(result.block_pos));
    wgpuQueueWriteBuffer(queue, model_uniform_buffer, 0, glm::value_ptr(model_matrix), sizeof(glm::mat4));

    wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, camera_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 1, model_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);

    const BlockFace all_faces[] = {
        BlockFace::PosX, BlockFace::NegX,
        BlockFace::PosY, BlockFace::NegY,
        BlockFace::PosZ, BlockFace::NegZ
    };

    for (const auto& face : all_faces) {
        glm::ivec3 neighbor_pos = result.block_pos + get_neighbor_offset(face);
        if (!chunk.is_solid(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z)) {
            uint32_t index_offset = static_cast<uint32_t>(to_cube_geometry_face(face)) * 6;
            wgpuRenderPassEncoderDrawIndexed(render_pass, 6, 1, index_offset, 0, 0);
        }
    }
}

} // namespace flint::graphics