#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "item.hpp"

namespace flint::ui
{
    struct UIVertex
    {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 tex_coords;
    };

    class ItemRenderer
    {
    public:
        ItemRenderer() = default;
        void initialize(
            wgpu::Device device,
            wgpu::TextureFormat texture_format);

        void render(
            wgpu::Device device,
            wgpu::Queue queue,
            wgpu::RenderPassEncoder render_pass,
            const std::vector<std::tuple<ItemStack, glm::vec2, float, glm::vec4>> &items);

    private:
        wgpu::RenderPipeline render_pipeline;
        wgpu::Buffer vertex_buffer;
        uint64_t buffer_size;
        wgpu::BindGroup projection_bind_group;
        wgpu::BindGroup ui_texture_bind_group;
    };
} // namespace flint::ui
