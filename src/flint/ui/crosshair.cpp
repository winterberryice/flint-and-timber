#include "crosshair.hpp"
#include <vector>
#include "glm/gtc/matrix_transform.hpp"

namespace {
    struct CrosshairVertex {
        glm::vec2 position;
    };
}

Crosshair::Crosshair(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height) {
    const float half_length = 10.0f;
    const float thickness = 1.0f;

    const std::vector<CrosshairVertex> vertices = {
        // Horizontal bar
        {{-half_length, -thickness}},
        {{ half_length, -thickness}},
        {{-half_length,  thickness}},
        {{ half_length, -thickness}},
        {{ half_length,  thickness}},
        {{-half_length,  thickness}},
        // Vertical bar
        {{-thickness, -half_length}},
        {{ thickness, -half_length}},
        {{-thickness,  half_length}},
        {{ thickness, -half_length}},
        {{ thickness,  half_length}},
        {{-thickness,  half_length}},
    };
    num_vertices = static_cast<uint32_t>(vertices.size());

    // ... vertex buffer, pipeline, and bind group creation ...
    // This will be filled in during the implementation phase.
}

void Crosshair::resize(uint32_t width, uint32_t height, wgpu::Queue queue) {
    if (width > 0 && height > 0) {
        projection_matrix = glm::ortho(
            -(width / 2.0f), static_cast<float>(width) / 2.0f,
            static_cast<float>(height) / 2.0f, -(static_cast<float>(height) / 2.0f),
            -1.0f, 1.0f
        );
        queue.writeBuffer(projection_buffer, 0, &projection_matrix, sizeof(glm::mat4));
    }
}

void Crosshair::draw(wgpu::RenderPassEncoder render_pass) {
    render_pass.setPipeline(render_pipeline);
    render_pass.setBindGroup(0, projection_bind_group, 0, nullptr);
    render_pass.setVertexBuffer(0, vertex_buffer);
    render_pass.draw(num_vertices, 1, 0, 0);
}
