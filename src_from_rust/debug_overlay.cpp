#include "debug_overlay.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace flint {

    // --- TODO ---
    // As with UIText, this class is a stub because it depends on a text rendering library (`wgpu_text` in Rust).
    // The rendering-related methods (`new`, `prepare`, `render`, `resize`) are empty placeholders.
    // The logic for updating the debug text content is preserved.

    DebugOverlay::DebugOverlay(WGPUDevice device, WGPUTextureFormat surface_format, WGPUTextureFormat depth_format, uint32_t width, uint32_t height) {
        (void)device; (void)surface_format; (void)depth_format; (void)width; (void)height;
        last_frame_time = std::chrono::steady_clock::now();
        std::cout << "TODO: DebugOverlay::new() - Text rendering is not implemented." << std::endl;
    }

    DebugOverlay::~DebugOverlay() {}

    void DebugOverlay::toggle_visibility() {
        visible = !visible;
    }

    void DebugOverlay::update(
        glm::vec3 player_position,
        std::optional<std::pair<glm::ivec3, const Block*>> selected_block,
        const Block* player_feet_block
    ) {
        if (!visible) {
            last_frame_time = std::chrono::steady_clock::now();
            frame_count = 0;
            accumulated_time = 0.0f;
            debug_text = "";
            return;
        }

        auto now = std::chrono::steady_clock::now();
        float delta_time = std::chrono::duration<float>(now - last_frame_time).count();
        last_frame_time = now;

        accumulated_time += delta_time;
        frame_count++;

        if (accumulated_time >= 1.0f) {
            fps = frame_count;
            frame_count = 0;
            accumulated_time -= 1.0f;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "FPS: " << fps << "\n";
        ss << "x: " << player_position.x << ", y: " << player_position.y << ", z: " << player_position.z;

        if (player_feet_block) {
            ss << "\nPlayer Light: " << (int)player_feet_block->sky_light << " sky, " << (int)player_feet_block->block_light << " block";
        }

        if (selected_block.has_value()) {
            const auto& [pos, block] = selected_block.value();
            // TODO: Need a way to get the string name of the block type.
            ss << "\n---\n";
            ss << "Looking at: BlockType(" << (int)block->block_type << ") at x: " << pos.x << ", y: " << pos.y << ", z: " << pos.z;
        }

        debug_text = ss.str();
    }

    void DebugOverlay::prepare(WGPUDevice device, WGPUQueue queue) {
        (void)device; (void)queue;
        // In a real implementation, this would take the `debug_text` string,
        // create text sections, and queue them with the text brush.
    }

    void DebugOverlay::render(WGPURenderPassEncoder render_pass) {
        (void)render_pass;
        // In a real implementation, this would call `brush.draw(render_pass)`.
        // For now, we can print to console as a fallback.
        // std::cout << debug_text << std::endl;
    }

    void DebugOverlay::resize(uint32_t width, uint32_t height, WGPUQueue queue) {
        (void)width; (void)height; (void)queue;
        // This would update the text brush's projection and bounds.
    }

} // namespace flint
