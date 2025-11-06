#include "inventory_ui_renderer.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_wgpu.h"

namespace flint::graphics
{

void InventoryUIRenderer::init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat)
{
    m_window = window;
    m_device = device;
    m_initialized = true;
}

void InventoryUIRenderer::cleanup()
{
    // ImGui context is shared with debug screen, so we don't destroy it here
    m_initialized = false;
}

void InventoryUIRenderer::process_event(const SDL_Event &event)
{
    // ImGui events are handled by debug screen renderer
}

void InventoryUIRenderer::begin_frame(int windowWidth, int windowHeight)
{
    if (!m_initialized)
        return;

    // NOTE: This method creates ImGui windows but doesn't call NewFrame/Render
    // Those are handled in the app's render loop to avoid conflicts with debug screen

    // Calculate dimensions
    const float inventoryCols = 9.0f;
    const float inventoryRows = 3.0f;
    const float hotbarCols = 9.0f;
    const float hotbarRows = 1.0f;

    const float slotTotalSize = SLOT_SIZE + SLOT_PADDING;
    const float inventoryWidth = inventoryCols * slotTotalSize + SLOT_PADDING;
    const float inventoryHeight = inventoryRows * slotTotalSize + SLOT_PADDING;
    const float hotbarWidth = hotbarCols * slotTotalSize + SLOT_PADDING;
    const float hotbarHeight = hotbarRows * slotTotalSize + SLOT_PADDING;

    const float totalHeight = inventoryHeight + GRID_SPACING + hotbarHeight;

    // Center position
    const float centerX = windowWidth / 2.0f;
    const float centerY = windowHeight / 2.0f;
    const float startX = centerX - inventoryWidth / 2.0f;
    const float startY = centerY - totalHeight / 2.0f;

    // Render dimmed background
    render_dimmed_background(windowWidth, windowHeight);

    // Render inventory grid (3x9)
    ImGui::SetNextWindowPos(ImVec2(startX, startY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inventoryWidth, inventoryHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.95f));
    ImGui::Begin("Inventory", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    render_inventory_grid(0, 0, 9, 3, "Inventory");

    ImGui::End();
    ImGui::PopStyleColor();

    // Render hotbar grid (1x9)
    const float hotbarStartY = startY + inventoryHeight + GRID_SPACING;
    ImGui::SetNextWindowPos(ImVec2(startX, hotbarStartY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.95f));
    ImGui::Begin("Hotbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    render_inventory_grid(0, 0, 9, 1, "Hotbar");

    ImGui::End();
    ImGui::PopStyleColor();
}

void InventoryUIRenderer::render(WGPURenderPassEncoder renderPass)
{
    // Rendering is handled by ImGui_ImplWGPU_RenderDrawData in the main render loop
    // This function is here for consistency with other renderers
}

void InventoryUIRenderer::render_dimmed_background(int windowWidth, int windowHeight)
{
    // Create a fullscreen dimmed overlay
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
    ImGui::Begin("DimmedBackground", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::End();
    ImGui::PopStyleColor();
}

void InventoryUIRenderer::render_inventory_grid(int startX, int startY, int cols, int rows, const char *label)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();

    const float slotTotalSize = SLOT_SIZE + SLOT_PADDING;

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            float x = window_pos.x + startX + SLOT_PADDING + col * slotTotalSize;
            float y = window_pos.y + startY + SLOT_PADDING + row * slotTotalSize;

            ImVec2 slot_min(x, y);
            ImVec2 slot_max(x + SLOT_SIZE, y + SLOT_SIZE);

            // Draw slot background
            draw_list->AddRectFilled(slot_min, slot_max, IM_COL32(60, 60, 60, 255));

            // Draw slot border
            draw_list->AddRect(slot_min, slot_max, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

            // Optional: Draw slot number for debugging
            // int slot_index = row * cols + col;
            // ImVec2 text_pos(x + SLOT_SIZE / 2.0f - 5.0f, y + SLOT_SIZE / 2.0f - 7.0f);
            // draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), std::to_string(slot_index).c_str());
        }
    }
}

} // namespace flint::graphics
