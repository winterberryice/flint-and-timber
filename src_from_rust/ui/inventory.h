#pragma once

#include "item.h"
#include "../input.h"
#include "../glm/glm.hpp"
#include <webgpu/webgpu.h>
#include <vector>
#include <array>
#include <optional>

namespace flint {
namespace ui {

    constexpr size_t INVENTORY_GRID_COLS = 9;
    constexpr size_t INVENTORY_GRID_ROWS = 3;
    constexpr size_t INVENTORY_NUM_SLOTS = INVENTORY_GRID_COLS * INVENTORY_GRID_ROWS;

    struct InventoryVertex {
        glm::vec2 position;
        glm::vec4 color;
    };

    class Inventory {
    public:
        Inventory(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height);
        ~Inventory();

        void draw(WGPURenderPassEncoder render_pass);

        void handle_mouse_click(const InputState& input, std::optional<ItemStack>& drag_item);

        std::array<std::optional<ItemStack>, INVENTORY_NUM_SLOTS> items;
        std::array<glm::vec2, INVENTORY_NUM_SLOTS> slot_positions;

    private:
        WGPUBuffer vertex_buffer = nullptr;
        uint32_t num_vertices = 0;
        WGPURenderPipeline render_pipeline = nullptr;
        WGPUBindGroup projection_bind_group = nullptr;
    };

} // namespace ui
} // namespace flint
