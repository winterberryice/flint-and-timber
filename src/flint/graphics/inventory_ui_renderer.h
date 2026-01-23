#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include "../inventory.h"

// Forward declarations
struct ImVec2;

namespace flint::graphics
{

class InventoryUIRenderer
{
public:
    InventoryUIRenderer() = default;
    ~InventoryUIRenderer() = default;

    void init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat);
    void cleanup();

    // Render full inventory UI (when inventory is open)
    void render_ui(int windowWidth, int windowHeight, const inventory::Inventory& inventory);

    // Render hotbar at bottom of screen (always visible during gameplay)
    void render_hotbar(int windowWidth, int windowHeight, const inventory::Inventory& inventory);

private:
    void render_inventory_grid(int startX, int startY, int cols, int rows, const char *label,
                               const inventory::Inventory* inventory, int slotOffset, int selectedSlot);
    void render_dimmed_background(int windowWidth, int windowHeight);
    void render_item_in_slot(ImVec2 slotMin, ImVec2 slotMax, const inventory::ItemStack& item);
    const char* get_block_name(BlockType type) const;

    SDL_Window *m_window = nullptr;
    WGPUDevice m_device = nullptr;
    bool m_initialized = false;

    // UI dimensions
    static constexpr float SLOT_SIZE = 50.0f;
    static constexpr float SLOT_PADDING = 5.0f;
    static constexpr float GRID_SPACING = 20.0f;
};

} // namespace flint::graphics
