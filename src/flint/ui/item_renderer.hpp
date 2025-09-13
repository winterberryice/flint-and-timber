#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "item.hpp"

struct UIVertex {
    glm::vec2 position;
    glm::vec4 color;
    glm::vec2 tex_coords;
};

class ItemRenderer {
public:
    ItemRenderer(
        wgpu::Device device,
        wgpu::TextureFormat texture_format,
        wgpu::BindGroupLayout projection_bind_group_layout,
        wgpu::BindGroupLayout ui_texture_bind_group_layout
    );

    void draw(
        wgpu::Device device,
        wgpu::Queue queue,
        wgpu::RenderPassEncoder render_pass,
        const wgpu::BindGroup& projection_bind_group,
        const wgpu::BindGroup& ui_texture_bind_group,
        const std::vector<std::tuple<ItemStack, glm::vec2, float, glm::vec4>>& items
    );

private:
    wgpu::RenderPipeline render_pipeline;
    wgpu::Buffer vertex_buffer;
    uint64_t buffer_size;
};
