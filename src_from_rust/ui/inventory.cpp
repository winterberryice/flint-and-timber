#include "inventory.h"
#include "../glm/gtc/matrix_transform.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

namespace flint {
namespace ui {

// TODO: This helper is duplicated in multiple files. Consolidate it.
std::string read_inventory_shader_file_content(const char* path) {
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
    std::cerr << "Failed to read file: " << path << std::endl;
    return "";
}

Inventory::Inventory(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height) {
    const float SLOT_SIZE = 50.0f;
    const float SLOT_MARGIN = 5.0f;
    const float TOTAL_SLOT_SIZE = SLOT_SIZE + SLOT_MARGIN;

    std::vector<InventoryVertex> vertices;

    float bg_width = (INVENTORY_GRID_COLS * TOTAL_SLOT_SIZE) + SLOT_MARGIN * 2.0f;
    float bg_height = (INVENTORY_GRID_ROWS * TOTAL_SLOT_SIZE) + SLOT_MARGIN * 2.0f;
    float bg_start_x = (width - bg_width) / 2.0f;
    float bg_start_y = (height - bg_height) / 2.0f;
    glm::vec4 bg_color = {0.1f, 0.1f, 0.1f, 0.8f};

    vertices.push_back({{bg_start_x, bg_start_y}, bg_color});
    vertices.push_back({{bg_start_x + bg_width, bg_start_y + bg_height}, bg_color});
    vertices.push_back({{bg_start_x, bg_start_y + bg_height}, bg_color});
    vertices.push_back({{bg_start_x, bg_start_y}, bg_color});
    vertices.push_back({{bg_start_x + bg_width, bg_start_y}, bg_color});
    vertices.push_back({{bg_start_x + bg_width, bg_start_y + bg_height}, bg_color});

    float grid_width = INVENTORY_GRID_COLS * TOTAL_SLOT_SIZE - SLOT_MARGIN;
    float grid_height = INVENTORY_GRID_ROWS * TOTAL_SLOT_SIZE - SLOT_MARGIN;
    float start_x = (width - grid_width) / 2.0f;
    float start_y = (height - grid_height) / 2.0f;
    glm::vec4 slot_color = {0.3f, 0.3f, 0.3f, 0.8f};

    for (size_t row = 0; row < INVENTORY_GRID_ROWS; ++row) {
        for (size_t col = 0; col < INVENTORY_GRID_COLS; ++col) {
            float x = start_x + col * TOTAL_SLOT_SIZE;
            float y = start_y + row * TOTAL_SLOT_SIZE;
            size_t index = row * INVENTORY_GRID_COLS + col;
            slot_positions[index] = {x + SLOT_SIZE / 2.0f, y + SLOT_SIZE / 2.0f};

            vertices.push_back({{x, y}, slot_color});
            vertices.push_back({{x + SLOT_SIZE, y + SLOT_SIZE}, slot_color});
            vertices.push_back({{x, y + SLOT_SIZE}, slot_color});
            vertices.push_back({{x, y}, slot_color});
            vertices.push_back({{x + SLOT_SIZE, y}, slot_color});
            vertices.push_back({{x + SLOT_SIZE, y + SLOT_SIZE}, slot_color});
        }
    }

    num_vertices = static_cast<uint32_t>(vertices.size());
    uint64_t vertex_buffer_size = num_vertices * sizeof(InventoryVertex);

    WGPUBufferDescriptor vertex_buffer_desc = { .label = "Inventory Vertex Buffer", .size = vertex_buffer_size, .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst, .mappedAtCreation = true };
    vertex_buffer = wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);
    void* mapping = wgpuBufferGetMappedRange(vertex_buffer, 0, vertex_buffer_size);
    memcpy(mapping, vertices.data(), vertex_buffer_size);
    wgpuBufferUnmap(vertex_buffer);

    glm::mat4 projection_matrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    WGPUBufferDescriptor proj_buffer_desc = { .label = "Inventory Projection Buffer", .size = sizeof(glm::mat4), .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst };
    WGPUBuffer projection_buffer = wgpuDeviceCreateBuffer(device, &proj_buffer_desc);
    wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), projection_buffer, 0, &projection_matrix, sizeof(glm::mat4));

    WGPUBindGroupLayoutEntry bgl_entry = { .binding = 0, .visibility = WGPUShaderStage_Vertex, .buffer.type = WGPUBufferBindingType_Uniform };
    WGPUBindGroupLayoutDescriptor bgl_desc = { .label = "Inventory BGL", .entryCount = 1, .entries = &bgl_entry };
    WGPUBindGroupLayout projection_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bgl_desc);

    WGPUBindGroupEntry bg_entry = { .binding = 0, .buffer = projection_buffer, .size = sizeof(glm::mat4) };
    WGPUBindGroupDescriptor bg_desc = { .label = "Inventory BG", .layout = projection_bind_group_layout, .entryCount = 1, .entries = &bg_entry };
    projection_bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);

    std::string shader_code = read_inventory_shader_file_content("src_from_rust/ui_shader.wgsl");
    WGPUShaderModuleWGSLDescriptor shader_wgsl_desc = { .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor, .source = shader_code.c_str() };
    WGPUShaderModuleDescriptor shader_desc = { .nextInChain = &shader_wgsl_desc.chain, .label = "UI Shader" };
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    WGPUPipelineLayoutDescriptor layout_desc = { .label = "Inventory Pipeline Layout", .bindGroupLayoutCount = 1, .bindGroupLayouts = &projection_bind_group_layout };
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPUVertexBufferLayout buffer_layout = {};
    buffer_layout.arrayStride = sizeof(InventoryVertex);
    buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
    std::vector<WGPUVertexAttribute> vert_attrs(2);
    vert_attrs[0] = { .format = WGPUVertexFormat_Float32x2, .offset = offsetof(InventoryVertex, position), .shaderLocation = 0 };
    vert_attrs[1] = { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(InventoryVertex, color), .shaderLocation = 1 };
    buffer_layout.attributeCount = vert_attrs.size();
    buffer_layout.attributes = vert_attrs.data();

    WGPUBlendState blend_state = { .color = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha }, .alpha = { .operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha } };
    WGPUColorTargetState color_target = { .format = surface_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All };
    WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = "fs_main", .targetCount = 1, .targets = &color_target };

    WGPURenderPipelineDescriptor pipeline_desc = {
        .label = "Inventory Render Pipeline",
        .layout = pipeline_layout,
        .vertex = { .module = shader_module, .entryPoint = "vs_main", .bufferCount = 1, .buffers = &buffer_layout },
        .fragment = &fragment_state,
        .primitive = { .topology = WGPUPrimitiveTopology_TriangleList, .frontFace = WGPUFrontFace_CCW },
    };
    render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

    // TODO: Manage lifetime of these resources properly.
    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuShaderModuleRelease(shader_module);
    wgpuBindGroupLayoutRelease(projection_bind_group_layout);
}

Inventory::~Inventory() {
    wgpuBufferRelease(vertex_buffer);
    wgpuRenderPipelineRelease(render_pipeline);
    wgpuBindGroupRelease(projection_bind_group);
}

void Inventory::draw(WGPURenderPassEncoder render_pass) {
    wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, projection_bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderDraw(render_pass, num_vertices, 1, 0, 0);
}

void Inventory::handle_mouse_click(const InputState& input, std::optional<ItemStack>& drag_item) {
    const float SLOT_SIZE = 50.0f;
    auto [cursor_x, cursor_y] = input.cursor_position;

    for (size_t i = 0; i < INVENTORY_NUM_SLOTS; ++i) {
        float slot_x = slot_positions[i].x - SLOT_SIZE / 2.0f;
        float slot_y = slot_positions[i].y - SLOT_SIZE / 2.0f;

        bool is_in_slot = cursor_x >= slot_x && cursor_x <= slot_x + SLOT_SIZE &&
                          cursor_y >= slot_y && cursor_y <= slot_y + SLOT_SIZE;

        if (is_in_slot) {
             if (input.left_mouse_pressed_this_frame) {
                if (!drag_item.has_value()) {
                    if (items[i].has_value()) {
                        drag_item = items[i];
                        items[i].reset();
                    }
                }
            } else if (input.left_mouse_released_this_frame) {
                if (drag_item.has_value()) {
                    auto d_item = drag_item.value();
                    drag_item.reset();

                    if (items[i].has_value()) {
                        auto& s_item = items[i].value();
                        if (s_item.item_type == d_item.item_type) {
                            uint8_t total = s_item.count + d_item.count;
                            s_item.count = std::min((uint8_t)64, total);
                            uint8_t remaining = (total > 64) ? (total - 64) : 0;
                            if (remaining > 0) {
                                d_item.count = remaining;
                                drag_item = d_item;
                            }
                        } else {
                            auto old_slot_item = items[i];
                            items[i] = d_item;
                            drag_item = old_slot_item;
                        }
                    } else {
                        items[i] = d_item;
                    }
                }
            } else if (input.right_mouse_released_this_frame) {
                if (drag_item.has_value()) {
                    auto& d_item = drag_item.value();
                    if (items[i].has_value()) {
                        auto& s_item = items[i].value();
                        if (s_item.item_type == d_item.item_type && s_item.count < 64) {
                            s_item.count++;
                            d_item.count--;
                            if (d_item.count == 0) drag_item.reset();
                        }
                    } else {
                        items[i] = ItemStack(d_item.item_type, 1);
                        d_item.count--;
                        if (d_item.count == 0) drag_item.reset();
                    }
                } else if (items[i].has_value()) {
                    auto& s_item = items[i].value();
                    if (s_item.count > 1) {
                        uint8_t half = (s_item.count + 1) / 2;
                        drag_item = ItemStack(s_item.item_type, half);
                        s_item.count -= half;
                        if (s_item.count == 0) items[i].reset();
                    }
                }
            }
            return;
        }
    }
}


} // namespace ui
} // namespace flint
