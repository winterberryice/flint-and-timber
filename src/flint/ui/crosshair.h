#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>

namespace flint::ui
{
    class Crosshair
    {
    public:
        Crosshair();
        ~Crosshair();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surface_format, uint32_t width, uint32_t height);
        void resize(uint32_t width, uint32_t height, WGPUQueue queue);
        void draw(WGPURenderPassEncoder render_pass) const;
        void cleanup();

    private:
        WGPUBuffer m_vertex_buffer = nullptr;
        uint32_t m_num_vertices = 0;
        WGPURenderPipeline m_render_pipeline = nullptr;

        glm::mat4 m_projection_matrix;
        WGPUBuffer m_projection_buffer = nullptr;
        WGPUBindGroup m_projection_bind_group = nullptr;
    };

} // namespace flint::ui