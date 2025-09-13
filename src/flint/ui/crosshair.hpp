#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>

namespace flint::ui
{
    class Crosshair
    {
    public:
        Crosshair() = default;
        void initialize(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height);
        void resize(uint32_t width, uint32_t height, wgpu::Queue queue);
        void render(wgpu::RenderPassEncoder render_pass, uint32_t width, uint32_t height);

    private:
        wgpu::Buffer vertex_buffer;
        uint32_t num_vertices;
        wgpu::RenderPipeline render_pipeline;
        glm::mat4 projection_matrix;
        wgpu::Buffer projection_buffer;
        wgpu::BindGroup projection_bind_group;
    };
} // namespace flint::ui
