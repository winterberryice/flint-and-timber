#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

namespace flint::graphics
{

class InventoryUIRenderer
{
public:
    InventoryUIRenderer() = default;
    ~InventoryUIRenderer() = default;

    void init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat);
    void cleanup();
    void process_event(const SDL_Event &event);
    void begin_frame(int windowWidth, int windowHeight);
    void render(WGPURenderPassEncoder renderPass);

private:
    void render_inventory_grid(int startX, int startY, int cols, int rows, const char *label);
    void render_dimmed_background(int windowWidth, int windowHeight);

    SDL_Window *m_window = nullptr;
    WGPUDevice m_device = nullptr;
    bool m_initialized = false;

    // UI dimensions
    static constexpr float SLOT_SIZE = 50.0f;
    static constexpr float SLOT_PADDING = 5.0f;
    static constexpr float GRID_SPACING = 20.0f;
};

} // namespace flint::graphics
