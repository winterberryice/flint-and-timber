#pragma once

#include "item.h"
#include "../glm/glm.hpp"
#include <webgpu/webgpu.h> // C-style Dawn header
#include <vector>
#include <cstdint>

namespace flint {
namespace ui {

    // Vertex format for UI elements.
    struct UIVertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 tex_coords;
    };

    class ItemRenderer {
    public:
        ItemRenderer(
            WGPUDevice device,
            WGPUTextureFormat surface_format,
            WGPUBindGroupLayout projection_bind_group_layout,
            WGPUBindGroupLayout ui_texture_bind_group_layout
        );
        ~ItemRenderer();

        void draw(
            WGPUDevice device,
            WGPUQueue queue,
            WGPURenderPassEncoder render_pass,
            WGPUBindGroup projection_bind_group,
            WGPUBindGroup ui_texture_bind_group,
            const std::vector<std::tuple<ItemStack, glm::vec2, float, glm::vec4>>& items
        );

    private:
        WGPURenderPipeline render_pipeline = nullptr;
        WGPUBuffer vertex_buffer = nullptr;
        uint64_t buffer_size = 0;
    };

    // Helper functions
    void generate_item_vertices(
        ItemType item_type,
        glm::vec2 position,
        float size,
        glm::vec4 color,
        std::vector<UIVertex>& vertices
    );

    void add_quad(
        std::vector<UIVertex>& vertices,
        const std::array<glm::vec2, 4>& points,
        const std::array<glm::vec2, 4>& uvs,
        glm::vec4 color
    );

} // namespace ui
} // namespace flint
