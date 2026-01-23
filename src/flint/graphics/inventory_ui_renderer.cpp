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

void InventoryUIRenderer::render_ui(int windowWidth, int windowHeight, inventory::Inventory& inventory)
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

    render_inventory_grid(0, 0, 9, 3, "Inventory", &inventory, 9, -1, windowWidth, windowHeight);

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

    render_inventory_grid(0, 0, 9, 1, "Hotbar", &inventory, 0, inventory.getSelectedHotbarSlot(), windowWidth, windowHeight);

    ImGui::End();
    ImGui::PopStyleColor();

    // Render held item at cursor
    ImGuiIO& io = ImGui::GetIO();
    render_held_item(io.MousePos.x, io.MousePos.y);
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

    render_inventory_grid(0, 0, 9, 1, "HotbarOverlay", const_cast<inventory::Inventory*>(&inventory), 0, inventory.getSelectedHotbarSlot(), windowWidth, windowHeight);

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
                                                 inventory::Inventory* inventory, int slotOffset, int selectedSlot,
                                                 int windowWidth, int windowHeight)
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

void InventoryUIRenderer::render_held_item(float mouseX, float mouseY)
{
    if (m_heldItem.isEmpty())
        return;

    ImDrawList *draw_list = ImGui::GetForegroundDrawList();

    // Render held item centered on cursor
    const float halfSize = SLOT_SIZE / 2.0f;
    ImVec2 slot_min(mouseX - halfSize, mouseY - halfSize);
    ImVec2 slot_max(mouseX + halfSize, mouseY + halfSize);

    // Semi-transparent background
    draw_list->AddRectFilled(slot_min, slot_max, IM_COL32(60, 60, 60, 200));
    draw_list->AddRect(slot_min, slot_max, IM_COL32(255, 255, 255, 200), 0.0f, 0, 2.0f);

    // Draw item
    const char* block_name = get_block_name(m_heldItem.type);
    ImVec2 text_size = ImGui::CalcTextSize(block_name);
    ImVec2 text_pos(
        mouseX - text_size.x / 2.0f,
        mouseY - text_size.y / 2.0f - 8.0f
    );
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), block_name);

    // Draw count
    if (m_heldItem.count > 1)
    {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", m_heldItem.count);
        ImVec2 count_size = ImGui::CalcTextSize(count_str);
        ImVec2 count_pos(
            slot_max.x - count_size.x - 3.0f,
            slot_max.y - count_size.y - 3.0f
        );
        draw_list->AddText(ImVec2(count_pos.x + 1, count_pos.y + 1), IM_COL32(0, 0, 0, 255), count_str);
        draw_list->AddText(count_pos, IM_COL32(255, 255, 255, 255), count_str);
    }
}

int InventoryUIRenderer::get_slot_at_position(float x, float y, int startX, int startY, int cols, int rows, int windowWidth, int windowHeight) const
{
    // This is used for the full inventory UI
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

    const float centerX = windowWidth / 2.0f;
    const float centerY = windowHeight / 2.0f;
    const float windowStartX = centerX - inventoryWidth / 2.0f;
    const float windowStartY = centerY - totalHeight / 2.0f;

    // Check main inventory (3x9) - slots 9-35
    float invStartX = windowStartX + SLOT_PADDING;
    float invStartY = windowStartY + SLOT_PADDING;

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 9; ++col)
        {
            float slotX = invStartX + col * slotTotalSize;
            float slotY = invStartY + row * slotTotalSize;

            if (x >= slotX && x < slotX + SLOT_SIZE &&
                y >= slotY && y < slotY + SLOT_SIZE)
            {
                return 9 + row * 9 + col; // Slots 9-35
            }
        }
    }

    // Check hotbar (1x9) - slots 0-8
    float hotbarStartY = windowStartY + inventoryHeight + GRID_SPACING;
    float hotbarStartX = windowStartX + SLOT_PADDING;

    for (int col = 0; col < 9; ++col)
    {
        float slotX = hotbarStartX + col * slotTotalSize;
        float slotY = hotbarStartY + SLOT_PADDING;

        if (x >= slotX && x < slotX + SLOT_SIZE &&
            y >= slotY && y < slotY + SLOT_SIZE)
        {
            return col; // Slots 0-8
        }
    }

    return -1; // No slot at this position
}

bool InventoryUIRenderer::handle_inventory_click(float mouseX, float mouseY, bool isLeftClick, inventory::Inventory& inventory, int windowWidth, int windowHeight)
{
    int clickedSlot = get_slot_at_position(mouseX, mouseY, 0, 0, 9, 4, windowWidth, windowHeight);

    if (clickedSlot < 0)
        return false; // Didn't click on a slot

    inventory::ItemStack& slotItem = inventory.getSlot(clickedSlot);

    if (isLeftClick)
    {
        // Left click: Swap entire stacks
        if (m_heldItem.isEmpty())
        {
            // Pick up entire stack
            m_heldItem = slotItem;
            slotItem = inventory::ItemStack(); // Empty the slot
        }
        else
        {
            // Try to place/merge
            if (slotItem.isEmpty())
            {
                // Place entire held stack
                slotItem = m_heldItem;
                m_heldItem = inventory::ItemStack();
            }
            else if (slotItem.type == m_heldItem.type)
            {
                // Merge stacks (simplified - no max stack size for now)
                slotItem.count += m_heldItem.count;
                m_heldItem = inventory::ItemStack();
            }
            else
            {
                // Swap different items
                inventory::ItemStack temp = m_heldItem;
                m_heldItem = slotItem;
                slotItem = temp;
            }
        }
    }
    else
    {
        // Right click: Split stacks or place one
        if (m_heldItem.isEmpty())
        {
            // Pick up half (rounded up)
            if (!slotItem.isEmpty() && slotItem.count > 0)
            {
                int halfCount = (slotItem.count + 1) / 2;
                m_heldItem = inventory::ItemStack(slotItem.type, halfCount);
                slotItem.count -= halfCount;

                if (slotItem.count <= 0)
                {
                    slotItem = inventory::ItemStack();
                }
            }
        }
        else
        {
            // Place one item
            if (slotItem.isEmpty())
            {
                slotItem = inventory::ItemStack(m_heldItem.type, 1);
                m_heldItem.count--;

                if (m_heldItem.count <= 0)
                {
                    m_heldItem = inventory::ItemStack();
                }
            }
            else if (slotItem.type == m_heldItem.type)
            {
                // Add one to existing stack
                slotItem.count++;
                m_heldItem.count--;

                if (m_heldItem.count <= 0)
                {
                    m_heldItem = inventory::ItemStack();
                }
            }
        }
    }

    return true; // Click was handled
}

} // namespace flint::graphics
