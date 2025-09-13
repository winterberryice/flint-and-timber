#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <optional>
#include <vector>
#include <tuple>
#include "world.hpp"
#include "raycast.hpp"

namespace flint
{
    struct WireframeVertex
    {
        glm::vec3 position;
    };

    class WireframeRenderer
    {
    public:
        WireframeRenderer() = default;
        void initialize(
            wgpu::Device device,
            wgpu::TextureFormat texture_format);

        void update_selection(std::optional<glm::ivec3> position);
        void render(wgpu::RenderPassEncoder render_pass, wgpu::Queue queue, const World &world);

    private:
        wgpu::RenderPipeline render_pipeline;
        wgpu::Buffer vertex_buffer;
        wgpu::Buffer index_buffer;
        std::vector<std::tuple<BlockFace, uint32_t, uint32_t>> face_render_info;
        wgpu::Buffer model_uniform_buffer;
        wgpu::BindGroup model_bind_group;
        glm::mat4 model_matrix;
        std::optional<glm::ivec3> current_selected_block_pos;
    };
} // namespace flint
