#include "inventory_ui_renderer.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_wgpu.h"
#include <cstdio>

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
    m_initialized = false;
}

void InventoryUIRenderer::render_ui(int windowWidth, int windowHeight, const inventory::Inventory& inventory)
{
    if (!m_initialized)
        return;

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

    // Render main inventory grid (3x9) - slots 9-35
    ImGui::SetNextWindowPos(ImVec2(startX, startY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inventoryWidth, inventoryHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.95f));
    ImGui::Begin("Inventory", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    render_inventory_grid(0, 0, 9, 3, "Inventory", &inventory, 9, -1);

    ImGui::End();
    ImGui::PopStyleColor();

    // Render hotbar grid (1x9) - slots 0-8
    const float hotbarStartY = startY + inventoryHeight + GRID_SPACING;
    ImGui::SetNextWindowPos(ImVec2(startX, hotbarStartY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.95f));
    ImGui::Begin("Hotbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    render_inventory_grid(0, 0, 9, 1, "Hotbar", &inventory, 0, inventory.getSelectedHotbarSlot());

    ImGui::End();
    ImGui::PopStyleColor();
}

void InventoryUIRenderer::render_hotbar(int windowWidth, int windowHeight, const inventory::Inventory& inventory)
{
    if (!m_initialized)
        return;

    const float hotbarCols = 9.0f;
    const float hotbarRows = 1.0f;

    const float slotTotalSize = SLOT_SIZE + SLOT_PADDING;
    const float hotbarWidth = hotbarCols * slotTotalSize + SLOT_PADDING;
    const float hotbarHeight = hotbarRows * slotTotalSize + SLOT_PADDING;

    // Position at bottom center
    const float centerX = windowWidth / 2.0f;
    const float startX = centerX - hotbarWidth / 2.0f;
    const float startY = windowHeight - hotbarHeight - 20.0f; // 20px from bottom

    ImGui::SetNextWindowPos(ImVec2(startX, startY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::Begin("HotbarOverlay", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoInputs);

    render_inventory_grid(0, 0, 9, 1, "HotbarOverlay", &inventory, 0, inventory.getSelectedHotbarSlot());

    ImGui::End();
    ImGui::PopStyleColor();
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

void InventoryUIRenderer::render_inventory_grid(int startX, int startY, int cols, int rows, const char *label,
                                                 const inventory::Inventory* inventory, int slotOffset, int selectedSlot)
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

            int slot_index = row * cols + col;
            int inventory_slot = slotOffset + slot_index;

            // Check if this is the selected slot
            bool is_selected = (selectedSlot >= 0 && slot_index == selectedSlot);

            // Draw slot background
            ImU32 bg_color = is_selected ? IM_COL32(100, 100, 100, 255) : IM_COL32(60, 60, 60, 255);
            draw_list->AddRectFilled(slot_min, slot_max, bg_color);

            // Draw slot border (highlighted if selected)
            ImU32 border_color = is_selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(100, 100, 100, 255);
            float border_thickness = is_selected ? 3.0f : 2.0f;
            draw_list->AddRect(slot_min, slot_max, border_color, 0.0f, 0, border_thickness);

            // Render item if inventory is provided and slot has an item
            if (inventory != nullptr)
            {
                const inventory::ItemStack& item = inventory->getSlot(inventory_slot);
                if (!item.isEmpty())
                {
                    render_item_in_slot(slot_min, slot_max, item);
                }
            }
        }
    }
}

void InventoryUIRenderer::render_item_in_slot(ImVec2 slotMin, ImVec2 slotMax, const inventory::ItemStack& item)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Draw block type name (centered)
    const char* block_name = get_block_name(item.type);
    ImVec2 text_size = ImGui::CalcTextSize(block_name);
    ImVec2 text_pos(
        slotMin.x + (SLOT_SIZE - text_size.x) / 2.0f,
        slotMin.y + (SLOT_SIZE - text_size.y) / 2.0f - 8.0f
    );
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), block_name);

    // Draw item count (bottom right corner)
    if (item.count > 1)
    {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", item.count);
        ImVec2 count_size = ImGui::CalcTextSize(count_str);
        ImVec2 count_pos(
            slotMax.x - count_size.x - 3.0f,
            slotMax.y - count_size.y - 3.0f
        );
        // Draw shadow for better readability
        draw_list->AddText(ImVec2(count_pos.x + 1, count_pos.y + 1), IM_COL32(0, 0, 0, 255), count_str);
        draw_list->AddText(count_pos, IM_COL32(255, 255, 255, 255), count_str);
    }
}

const char* InventoryUIRenderer::get_block_name(BlockType type) const
{
    switch (type)
    {
        case BlockType::Air: return "Air";
        case BlockType::Dirt: return "Dirt";
        case BlockType::Grass: return "Grass";
        case BlockType::OakLog: return "Oak";
        case BlockType::OakLeaves: return "Leaves";
        default: return "Unknown";
    }
}

} // namespace flint::graphics
