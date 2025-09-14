#include "item_renderer.h"
#include "../block.h" // For Block::new and get_texture_atlas_indices
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

// TODO: These constants should probably be defined in a more central place.
const float ATLAS_WIDTH_IN_BLOCKS = 16.0f;
const float ATLAS_HEIGHT_IN_BLOCKS = 1.0f; // This seems wrong, likely should be > 1. Using 1.0 based on Rust code.

namespace flint {
namespace ui {

// Helper to read a file, needed for the shader.
// TODO: This is a temporary utility. A better file utility should be used.
std::string read_file_content(const char* path) {
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


ItemRenderer::ItemRenderer(
    WGPUDevice device,
    WGPUTextureFormat surface_format,
    WGPUBindGroupLayout projection_bind_group_layout,
    WGPUBindGroupLayout ui_texture_bind_group_layout
) {
    // --- Shader Module ---
    // TODO: The path to the shader is hardcoded here. This should be handled by a resource manager.
    std::string shader_code = read_file_content("src_from_rust/item_ui_shader.wgsl");
    WGPUShaderModuleWGSLDescriptor shader_wgsl_desc = {};
    shader_wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_wgsl_desc.source = shader_code.c_str();

    WGPUShaderModuleDescriptor shader_desc = {};
    shader_desc.nextInChain = &shader_wgsl_desc.chain;
    shader_desc.label = "Item Renderer Shader";
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    // --- Render Pipeline ---
    WGPUPipelineLayoutDescriptor layout_desc = {};
    layout_desc.label = "Item Renderer Pipeline Layout";
    std::vector<WGPUBindGroupLayout> bind_group_layouts = { projection_bind_group_layout, ui_texture_bind_group_layout };
    layout_desc.bindGroupLayoutCount = bind_group_layouts.size();
    layout_desc.bindGroupLayouts = bind_group_layouts.data();
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.label = "Item Renderer Pipeline";
    pipeline_desc.layout = pipeline_layout;

    // Vertex State
    std::vector<WGPUVertexAttribute> vertex_attributes(3);
    // pos
    vertex_attributes[0].shaderLocation = 0;
    vertex_attributes[0].offset = offsetof(UIVertex, position);
    vertex_attributes[0].format = WGPUVertexFormat_Float32x2;
    // color
    vertex_attributes[1].shaderLocation = 1;
    vertex_attributes[1].offset = offsetof(UIVertex, color);
    vertex_attributes[1].format = WGPUVertexFormat_Float32x4;
    // tex_coords
    vertex_attributes[2].shaderLocation = 2;
    vertex_attributes[2].offset = offsetof(UIVertex, tex_coords);
    vertex_attributes[2].format = WGPUVertexFormat_Float32x2;

    WGPUVertexBufferLayout buffer_layout = {};
    buffer_layout.arrayStride = sizeof(UIVertex);
    buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
    buffer_layout.attributeCount = vertex_attributes.size();
    buffer_layout.attributes = vertex_attributes.data();

    pipeline_desc.vertex.module = shader_module;
    pipeline_desc.vertex.entryPoint = "vs_main";
    pipeline_desc.vertex.bufferCount = 1;
    pipeline_desc.vertex.buffers = &buffer_layout;

    // Fragment State
    WGPUColorTargetState color_target = {};
    color_target.format = surface_format;
    color_target.writeMask = WGPUColorWriteMask_All;
    // Alpha blending
    WGPUBlendState blend_state = {};
    blend_state.color.operation = WGPUBlendOperation_Add;
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.alpha.operation = WGPUBlendOperation_Add;
    blend_state.alpha.srcFactor = WGPUBlendFactor_One;
    blend_state.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    color_target.blend = &blend_state;

    WGPUFragmentState fragment_state = {};
    fragment_state.module = shader_module;
    fragment_state.entryPoint = "fs_main";
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;
    pipeline_desc.fragment = &fragment_state;

    // Other state
    pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
    // TODO: Verify other primitive, depth_stencil, multisample states if needed.

    render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

    // --- Vertex Buffer ---
    buffer_size = 1024 * 4; // Initial size
    WGPUBufferDescriptor buffer_desc = {};
    buffer_desc.label = "Item Renderer Vertex Buffer";
    buffer_desc.size = buffer_size;
    buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    buffer_desc.mappedAtCreation = false;
    vertex_buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);

    // Release resources that are no longer needed
    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuShaderModuleRelease(shader_module);
}

ItemRenderer::~ItemRenderer() {
    wgpuRenderPipelineRelease(render_pipeline);
    wgpuBufferRelease(vertex_buffer);
}

void ItemRenderer::draw(
    WGPUDevice device,
    WGPUQueue queue,
    WGPURenderPassEncoder render_pass,
    WGPUBindGroup projection_bind_group,
    WGPUBindGroup ui_texture_bind_group,
    const std::vector<std::tuple<ItemStack, glm::vec2, float, glm::vec4>>& items
) {
    std::vector<UIVertex> vertices;
    for (const auto& item_tuple : items) {
        generate_item_vertices(
            std::get<0>(item_tuple).item_type,
            std::get<1>(item_tuple),
            std::get<2>(item_tuple),
            std::get<3>(item_tuple),
            vertices
        );
    }

    if (vertices.empty()) {
        return;
    }

    uint64_t required_size = vertices.size() * sizeof(UIVertex);
    if (required_size > buffer_size) {
        // Resize buffer
        wgpuBufferRelease(vertex_buffer);
        // TODO: next_power_of_two logic from rust
        buffer_size = required_size * 2;

        WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.label = "Item Renderer Vertex Buffer (Resized)";
        buffer_desc.size = buffer_size;
        buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        buffer_desc.mappedAtCreation = false;
        vertex_buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);
    }

    wgpuQueueWriteBuffer(queue, vertex_buffer, 0, vertices.data(), required_size);

    wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, projection_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 1, ui_texture_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, required_size);
    wgpuRenderPassEncoderDraw(render_pass, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}

void generate_item_vertices(
    ItemType item_type,
    glm::vec2 position,
    float size,
    glm::vec4 color,
    std::vector<UIVertex>& vertices
) {
    Block temp_block(item_type.block_type);
    auto indices = temp_block.get_texture_atlas_indices();
    auto uv_top_arr = indices[4];
    auto uv_side_arr = indices[0];
    auto uv_front_arr = indices[2];

    float texel_width = 1.0f / ATLAS_WIDTH_IN_BLOCKS;
    float texel_height = 1.0f / ATLAS_HEIGHT_IN_BLOCKS;

    glm::vec2 uv_top(uv_top_arr[0] * texel_width, uv_top_arr[1] * texel_height);
    glm::vec2 uv_side(uv_side_arr[0] * texel_width, uv_side_arr[1] * texel_height);
    glm::vec2 uv_front(uv_front_arr[0] * texel_width, uv_front_arr[1] * texel_height);

    std::array<glm::vec2, 4> top_face_uv = { uv_top, {uv_top.x + texel_width, uv_top.y}, {uv_top.x + texel_width, uv_top.y + texel_height}, {uv_top.x, uv_top.y + texel_height} };
    std::array<glm::vec2, 4> side_face_uv = { uv_side, {uv_side.x + texel_width, uv_side.y}, {uv_side.x + texel_width, uv_side.y + texel_height}, {uv_side.x, uv_side.y + texel_height} };
    std::array<glm::vec2, 4> front_face_uv = { uv_front, {uv_front.x + texel_width, uv_front.y}, {uv_front.x + texel_width, uv_front.y + texel_height}, {uv_front.x, uv_front.y + texel_height} };

    float s = size / 2.0f;
    float y_squish = 0.5f;
    position.y += size / 4.0f;

    glm::vec2 p1 = {position.x, position.y + s * y_squish};
    glm::vec2 p2 = {position.x + s, position.y};
    glm::vec2 p3 = {position.x, position.y - s * y_squish};
    glm::vec2 p4 = {position.x - s, position.y};
    add_quad(vertices, {p1, p2, p3, p4}, top_face_uv, color);

    glm::vec2 p5 = p4;
    glm::vec2 p6 = p3;
    glm::vec2 p7 = {p3.x, p3.y - s};
    glm::vec2 p8 = {p4.x, p4.y - s};
    add_quad(vertices, {p5, p6, p7, p8}, side_face_uv, color);

    glm::vec2 p9 = p3;
    glm::vec2 p10 = p2;
    glm::vec2 p11 = {p2.x, p2.y - s};
    glm::vec2 p12 = {p3.x, p3.y - s};
    add_quad(vertices, {p9, p10, p11, p12}, front_face_uv, color);
}

void add_quad(
    std::vector<UIVertex>& vertices,
    const std::array<glm::vec2, 4>& points,
    const std::array<glm::vec2, 4>& uvs,
    glm::vec4 color
) {
    vertices.push_back({points[0], color, uvs[0]});
    vertices.push_back({points[3], color, uvs[3]});
    vertices.push_back({points[1], color, uvs[1]});
    vertices.push_back({points[1], color, uvs[1]});
    vertices.push_back({points[3], color, uvs[3]});
    vertices.push_back({points[2], color, uvs[2]});
}


} // namespace ui
} // namespace flint
