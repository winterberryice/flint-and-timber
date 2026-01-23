#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include "../graphics/debug_screen_renderer.h"
#include "../graphics/inventory_ui_renderer.h"
#include "../player.h"
#include "../world.h"

namespace flint::ui
{
    // Centralized UI manager that owns all UI renderers and manages ImGui lifecycle
    class UIManager
    {
    public:
        UIManager() = default;
        ~UIManager() = default;

        void init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat);
        void cleanup();
        void process_event(const SDL_Event &event);

        // Render all active UI elements
        // This manages the ImGui frame lifecycle internally
        void render(
            bool showDebugScreen,
            bool showInventory,
            player::Player &player,
            const World &world,
            int windowWidth,
            int windowHeight
        );

        // Handle mouse click on inventory UI (returns true if handled)
        bool handle_inventory_click(float mouseX, float mouseY, bool isLeftClick, player::Player &player, int windowWidth, int windowHeight);

        // Render the ImGui draw data to the render pass
        void render_to_pass(WGPURenderPassEncoder renderPass);

    private:
        graphics::DebugScreenRenderer m_debugScreenRenderer;
        graphics::InventoryUIRenderer m_inventoryUIRenderer;

        bool m_initialized = false;
        bool m_hasActiveFrame = false;
    };

} // namespace flint::ui
