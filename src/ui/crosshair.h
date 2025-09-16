#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu.h>
#include <cstdint>

#include "flint_utils.h"

namespace flint
{
    namespace ui
    {

        struct CrosshairVertex
        {
            glm::vec2 position;
        };

        class Crosshair
        {
        public:
            Crosshair(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height);
            ~Crosshair();

            void resize(uint32_t new_width, uint32_t new_height, WGPUQueue queue);

            void draw(WGPURenderPassEncoder render_pass);

        private:
            WGPUBuffer vertex_buffer = nullptr;
            uint32_t num_vertices = 0;
            WGPURenderPipeline render_pipeline = nullptr;

            // Projection-related members
            glm::mat4 projection_matrix;
            WGPUBuffer projection_buffer = nullptr;
            WGPUBindGroupLayout projection_bind_group_layout = nullptr;
            WGPUBindGroup projection_bind_group = nullptr;
        };

    } // namespace ui
} // namespace flint
