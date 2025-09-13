#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <optional>
#include <array>
#include "item.hpp"
#include "../input.hpp"

const int NUM_INVENTORY_SLOTS = 27; // 9x3

class Inventory {
public:
    wgpu::Buffer vertex_buffer;
    uint32_t num_vertices;
    wgpu::RenderPipeline render_pipeline;
    wgpu::BindGroup projection_bind_group;
    std::array<std::optional<ItemStack>, NUM_INVENTORY_SLOTS> items;
    std::array<glm::vec2, NUM_INVENTORY_SLOTS> slot_positions;

    Inventory(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height);
    void draw(wgpu::RenderPassEncoder render_pass);
    void handle_mouse_click(const InputState& input, std::optional<ItemStack>& drag_item);
};
