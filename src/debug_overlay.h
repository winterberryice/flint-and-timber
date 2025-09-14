#pragma once

#include "block.h"
#include "glm/glm.hpp"
#include <webgpu/webgpu.h>
#include <string>
#include <optional>
#include <chrono>

namespace flint {

    class DebugOverlay {
    public:
        DebugOverlay(WGPUDevice device, WGPUTextureFormat surface_format, WGPUTextureFormat depth_format, uint32_t width, uint32_t height);
        ~DebugOverlay();

        void toggle_visibility();

        void update(
            glm::vec3 player_position,
            std::optional<std::pair<glm::ivec3, const Block*>> selected_block,
            const Block* player_feet_block
        );

        // Stubs for rendering functionality
        void prepare(WGPUDevice device, WGPUQueue queue);
        void render(WGPURenderPassEncoder render_pass);
        void resize(uint32_t width, uint32_t height, WGPUQueue queue);

    private:
        bool visible = true;

        // FPS calculation
        std::chrono::steady_clock::time_point last_frame_time;
        uint32_t frame_count = 0;
        float accumulated_time = 0.0f;
        uint32_t fps = 0;

        // The text to be rendered
        std::string debug_text;

        // TODO: This class is a stub for text rendering.
        // The original Rust code uses the `wgpu_text` library. A C++ equivalent is needed.
        // Members for a text brush, pipeline, etc. would go here.
    };

} // namespace flint
