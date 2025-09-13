#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>

class Crosshair {
public:
    wgpu::Buffer vertex_buffer;
    uint32_t num_vertices;
    wgpu::RenderPipeline render_pipeline;
    glm::mat4 projection_matrix;
    wgpu::Buffer projection_buffer;
    wgpu::BindGroup projection_bind_group;

    Crosshair(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height);
    void resize(uint32_t width, uint32_t height, wgpu::Queue queue);
    void draw(wgpu::RenderPassEncoder render_pass);
};
