#include "ui_manager.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>

namespace flint::ui
{

void UIManager::init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat)
{
    // Initialize ImGui context (shared by all UI elements)
    m_debugScreenRenderer.init(window, device, surfaceFormat);
    m_inventoryUIRenderer.init(window, device, surfaceFormat);

    m_initialized = true;
}

void UIManager::cleanup()
{
    m_inventoryUIRenderer.cleanup();
    m_debugScreenRenderer.cleanup();
    m_initialized = false;
}

void UIManager::process_event(const SDL_Event &event)
{
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void UIManager::render(
    bool showDebugScreen,
    bool showInventory,
    const player::Player &player,
    const World &world,
    int windowWidth,
    int windowHeight
)
{
    if (!m_initialized)
        return;

    // Always need ImGui if we're showing any UI (debug screen, inventory, or hotbar)
    bool needsImGui = showDebugScreen || showInventory || true; // Always true for hotbar
    if (!needsImGui)
    {
        m_hasActiveFrame = false;
        return;
    }

    // Start ImGui frame (ONCE for all UI elements)
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Render all active UI elements
    if (showDebugScreen)
    {
        m_debugScreenRenderer.render_ui(player, world);
    }

    if (showInventory)
    {
        // Render full inventory UI
        m_inventoryUIRenderer.render_ui(windowWidth, windowHeight, player.get_inventory());
    }
    else
    {
        // Always render hotbar during gameplay
        m_inventoryUIRenderer.render_hotbar(windowWidth, windowHeight, player.get_inventory());
    }

    // Finalize ImGui frame (ONCE for all UI elements)
    ImGui::Render();
    m_hasActiveFrame = true;
}

void UIManager::render_to_pass(WGPURenderPassEncoder renderPass)
{
    if (m_hasActiveFrame)
    {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
    }
}

} // namespace flint::ui
