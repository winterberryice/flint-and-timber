#pragma once

#include "item.h"
#include "../input.h"
#include <glm/glm.hpp>
#include <webgpu/webgpu.h>
#include <vector>
#include <array>
#include <optional>

namespace flint
{
    namespace ui
    {

        constexpr size_t NUM_SLOTS = 9;

        struct HotbarVertex
        {
            glm::vec2 position;
            glm::vec4 color;
        };

        class Hotbar
        {
        public:
            Hotbar(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height);
            ~Hotbar();

            void draw(WGPURenderPassEncoder render_pass);

            void handle_mouse_click(const InputState &input, std::optional<ItemStack> &drag_item);

            std::array<std::optional<ItemStack>, NUM_SLOTS> items;
            std::array<glm::vec2, NUM_SLOTS> slot_positions;

        private:
            WGPUBuffer vertex_buffer = nullptr;
            uint32_t num_vertices = 0;
            WGPURenderPipeline render_pipeline = nullptr;
            WGPUBindGroup projection_bind_group = nullptr;
            // TODO: The projection buffer and layout are not stored here but are owned by the object.
            // This is a simplification. A real implementation would manage their lifetime.
        };

    } // namespace ui
} // namespace flint
