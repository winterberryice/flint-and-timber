#include "wireframe_renderer.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <tuple>

namespace flint {

// --- Geometry Generation ---

namespace { // Anonymous namespace for internal linkage

void create_strip_quad(
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
    std::vector<WireframeVertex>& vertices,
    std::vector<uint16_t>& indices
) {
    uint16_t base_vertex_index = static_cast<uint16_t>(vertices.size());
    vertices.push_back({p0});
    vertices.push_back({p1});
    vertices.push_back({p2});
    vertices.push_back({p3});
    indices.insert(indices.end(), {
        base_vertex_index, (uint16_t)(base_vertex_index + 1), (uint16_t)(base_vertex_index + 2),
        base_vertex_index, (uint16_t)(base_vertex_index + 2), (uint16_t)(base_vertex_index + 3)
    });
}

void generate_quads_for_face(
    glm::vec3 face_center_offset, glm::vec3 axis1, glm::vec3 axis2,
    std::vector<WireframeVertex>& vertices,
    std::vector<uint16_t>& indices
) {
    const float m = 0.00f;
    const float t = 0.02f;
    const float h1 = 0.5f;
    const float h2 = 0.5f;

    // Quad 1
    glm::vec3 q1p0 = face_center_offset - axis2 * h2 + axis1 * (-h1 + m) + axis2 * m;
    glm::vec3 q1p1 = face_center_offset - axis2 * h2 + axis1 * (h1 - m) + axis2 * m;
    glm::vec3 q1p2 = face_center_offset - axis2 * h2 + axis1 * (h1 - m) + axis2 * (m + t);
    glm::vec3 q1p3 = face_center_offset - axis2 * h2 + axis1 * (-h1 + m) + axis2 * (m + t);
    create_strip_quad(q1p0, q1p1, q1p2, q1p3, vertices, indices);

    // Quad 2
    glm::vec3 q2p0 = face_center_offset + axis2 * h2 + axis1 * (-h1 + m) - axis2 * (m + t);
    glm::vec3 q2p1 = face_center_offset + axis2 * h2 + axis1 * (h1 - m) - axis2 * (m + t);
    glm::vec3 q2p2 = face_center_offset + axis2 * h2 + axis1 * (h1 - m) - axis2 * m;
    glm::vec3 q2p3 = face_center_offset + axis2 * h2 + axis1 * (-h1 + m) - axis2 * m;
    create_strip_quad(q2p0, q2p1, q2p2, q2p3, vertices, indices);

    // Quad 3
    glm::vec3 q3p0 = face_center_offset - axis1 * h1 + axis2 * (-h2 + m + t) + axis1 * m;
    glm::vec3 q3p1 = face_center_offset - axis1 * h1 + axis2 * (h2 - m - t) + axis1 * m;
    glm::vec3 q3p2 = face_center_offset - axis1 * h1 + axis2 * (h2 - m - t) + axis1 * (m + t);
    glm::vec3 q3p3 = face_center_offset - axis1 * h1 + axis2 * (-h2 + m + t) + axis1 * (m + t);
    create_strip_quad(q3p0, q3p1, q3p2, q3p3, vertices, indices);

    // Quad 4
    glm::vec3 q4p0 = face_center_offset + axis1 * h1 + axis2 * (-h2 + m + t) - axis1 * (m + t);
    glm::vec3 q4p1 = face_center_offset + axis1 * h1 + axis2 * (h2 - m - t) - axis1 * (m + t);
    glm::vec3 q4p2 = face_center_offset + axis1 * h1 + axis2 * (h2 - m - t) - axis1 * m;
    glm::vec3 q4p3 = face_center_offset + axis1 * h1 + axis2 * (-h2 + m + t) - axis1 * m;
    create_strip_quad(q4p0, q4p1, q4p2, q4p3, vertices, indices);
}

// Equivalent of lazy_static
const auto& get_face_quads_cube_geometry() {
    static const auto geometry = []{
        std::vector<WireframeVertex> all_vertices;
        std::vector<uint16_t> all_indices;
        std::vector<WireframeRenderer::FaceRenderInfo> face_render_info;

        auto generate_and_record = [&](BlockFace face, glm::vec3 center_offset, glm::vec3 axis1, glm::vec3 axis2) {
            uint32_t start_index_count = all_indices.size();
            generate_quads_for_face(center_offset, axis1, axis2, all_vertices, all_indices);
            uint32_t num_new_indices = all_indices.size() - start_index_count;
            if (num_new_indices > 0) {
                face_render_info.push_back({face, start_index_count, num_new_indices});
            }
        };

        generate_and_record(BlockFace::PosX, {1.0f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
        generate_and_record(BlockFace::NegX, {0.0f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
        generate_and_record(BlockFace::PosY, {0.5f, 1.0f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
        generate_and_record(BlockFace::NegY, {0.5f, 0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
        generate_and_record(BlockFace::PosZ, {0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        generate_and_record(BlockFace::NegZ, {0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        return std::make_tuple(all_vertices, all_indices, face_render_info);
    }();
    return geometry;
}

// TODO: This helper is duplicated. Consolidate it.
std::string read_wireframe_shader_file_content(const char* path) {
    std::ifstream file(path);
    if (file) {
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    std::cerr << "Failed to read file: " << path << std::endl;
    return "";
}

} // anonymous namespace

WireframeRenderer::WireframeRenderer(
    WGPUDevice device,
    WGPUTextureFormat surface_format,
    WGPUTextureFormat depth_format,
    WGPUBindGroupLayout camera_bind_group_layout
) {
    auto& geometry = get_face_quads_cube_geometry();
    const auto& vertices = std::get<0>(geometry);
    const auto& indices = std::get<1>(geometry);
    face_render_info = std::get<2>(geometry);

    // Create vertex and index buffers
    WGPUBufferDescriptor vb_desc = { .label = "Wireframe VB", .size = vertices.size() * sizeof(WireframeVertex), .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst, .mappedAtCreation = true };
    vertex_buffer = wgpuDeviceCreateBuffer(device, &vb_desc);
    memcpy(wgpuBufferGetMappedRange(vertex_buffer, 0, WGPU_WHOLE_SIZE), vertices.data(), vb_desc.size);
    wgpuBufferUnmap(vertex_buffer);

    WGPUBufferDescriptor ib_desc = { .label = "Wireframe IB", .size = indices.size() * sizeof(uint16_t), .usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst, .mappedAtCreation = true };
    index_buffer = wgpuDeviceCreateBuffer(device, &ib_desc);
    memcpy(wgpuBufferGetMappedRange(index_buffer, 0, WGPU_WHOLE_SIZE), indices.data(), ib_desc.size);
    wgpuBufferUnmap(index_buffer);

    // Create model uniform buffer and bind group
    model_data.model_matrix = glm::mat4(1.0f);
    WGPUBufferDescriptor model_buff_desc = { .label = "Wireframe Model UB", .size = sizeof(ModelUniformData), .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst };
    model_uniform_buffer = wgpuDeviceCreateBuffer(device, &model_buff_desc);

    WGPUBindGroupLayoutEntry model_bgl_entry = { .binding = 0, .visibility = WGPUShaderStage_Vertex, .buffer.type = WGPUBufferBindingType_Uniform };
    WGPUBindGroupLayoutDescriptor model_bgl_desc = { .label = "Wireframe Model BGL", .entryCount = 1, .entries = &model_bgl_entry };
    WGPUBindGroupLayout model_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &model_bgl_desc);

    WGPUBindGroupEntry model_bg_entry = { .binding = 0, .buffer = model_uniform_buffer, .size = sizeof(ModelUniformData) };
    WGPUBindGroupDescriptor model_bg_desc = { .label = "Wireframe Model BG", .layout = model_bind_group_layout, .entryCount = 1, .entries = &model_bg_entry };
    model_bind_group = wgpuDeviceCreateBindGroup(device, &model_bg_desc);

    // Create pipeline
    std::string shader_code = read_wireframe_shader_file_content("src_from_rust/wireframe_shader.wgsl");
    WGPUShaderModuleWGSLDescriptor shader_wgsl_desc = { .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor, .source = shader_code.c_str() };
    WGPUShaderModuleDescriptor shader_desc = { .nextInChain = &shader_wgsl_desc.chain, .label = "Wireframe Shader" };
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    std::vector<WGPUBindGroupLayout> bgls = { camera_bind_group_layout, model_bind_group_layout };
    WGPUPipelineLayoutDescriptor layout_desc = { .label = "Wireframe Pipeline Layout", .bindGroupLayoutCount = (uint32_t)bgls.size(), .bindGroupLayouts = bgls.data() };
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPUVertexBufferLayout vert_layout = {};
    vert_layout.arrayStride = sizeof(WireframeVertex);
    vert_layout.stepMode = WGPUVertexStepMode_Vertex;
    WGPUVertexAttribute vert_attr = { .format = WGPUVertexFormat_Float32x3, .offset = 0, .shaderLocation = 0 };
    vert_layout.attributeCount = 1;
    vert_layout.attributes = &vert_attr;

    WGPUBlendState blend_state = { .color = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha }, .alpha = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha } };
    WGPUColorTargetState color_target = { .format = surface_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All };
    WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = "fs_main", .targetCount = 1, .targets = &color_target };

    WGPUDepthStencilState depth_stencil_state = {};
    depth_stencil_state.format = depth_format;
    depth_stencil_state.depthWriteEnabled = true;
    depth_stencil_state.depthCompare = WGPUCompareFunction_LessEqual;
    depth_stencil_state.bias.constant = -2;
    depth_stencil_state.bias.slopeScale = -2.0f;

    WGPURenderPipelineDescriptor pipeline_desc = {
        .label = "Wireframe Pipeline", .layout = pipeline_layout,
        .vertex = { .module = shader_module, .entryPoint = "vs_main", .bufferCount = 1, .buffers = &vert_layout },
        .fragment = &fragment_state,
        .depthStencil = &depth_stencil_state,
        .primitive = { .topology = WGPUPrimitiveTopology_TriangleList, .frontFace = WGPUFrontFace_CCW }
    };
    render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

    wgpuShaderModuleRelease(shader_module);
    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuBindGroupLayoutRelease(model_bind_group_layout);
}

WireframeRenderer::~WireframeRenderer() {
    wgpuRenderPipelineRelease(render_pipeline);
    wgpuBufferRelease(vertex_buffer);
    wgpuBufferRelease(index_buffer);
    wgpuBufferRelease(model_uniform_buffer);
    wgpuBindGroupRelease(model_bind_group);
}

void WireframeRenderer::update_selection(std::optional<glm::ivec3> position) {
    current_selected_block_pos = position;
    if (position.has_value()) {
        model_data.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(position.value()));
    }
}

namespace {
    glm::ivec3 get_neighbor_offset(BlockFace face) {
        switch (face) {
            case BlockFace::PosX: return {1, 0, 0};
            case BlockFace::NegX: return {-1, 0, 0};
            case BlockFace::PosY: return {0, 1, 0};
            case BlockFace::NegY: return {0, -1, 0};
            case BlockFace::PosZ: return {0, 0, 1};
            case BlockFace::NegZ: return {0, 0, -1};
        }
        return {0,0,0}; // Should not happen
    }
}

void WireframeRenderer::draw(WGPURenderPassEncoder render_pass, WGPUQueue queue, const World& world) {
    if (!current_selected_block_pos.has_value()) return;

    wgpuQueueWriteBuffer(queue, model_uniform_buffer, 0, &model_data, sizeof(ModelUniformData));

    wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
    // Bind group 0 (camera) is assumed to be set by the main render loop
    wgpuRenderPassEncoderSetBindGroup(render_pass, 1, model_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);

    glm::ivec3 selected_pos = current_selected_block_pos.value();

    for (const auto& info : face_render_info) {
        if (info.index_count == 0) continue;

        glm::ivec3 neighbor_pos = selected_pos + get_neighbor_offset(info.face);

        bool should_draw_face = true;
        const Block* neighbor_block = world.get_block_at_world((float)neighbor_pos.x, (float)neighbor_pos.y, (float)neighbor_pos.z);
        if (neighbor_block && neighbor_block->is_solid()) {
            should_draw_face = false;
        }

        if (should_draw_face) {
            wgpuRenderPassEncoderDrawIndexed(render_pass, info.index_count, 1, info.index_offset, 0, 0);
        }
    }
}

} // namespace flint
