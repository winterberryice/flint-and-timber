#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#include "camera.h"
#include "chunk.h"
#include "camera.h"
#include "graphics/world_renderer.h"
#include "graphics/selection_renderer.h"
#include "graphics/crosshair_renderer.h"
#include "graphics/debug_screen_renderer.h"
#include "graphics/inventory_ui_renderer.h"
#include "player.h"
#include "game_state.h"

namespace flint
{

    class App
    {
    public:
        App();
        ~App();

        void run();

    private:
        void render();
        void update_camera();
        void onResize(int width, int height);
        void handle_input_event(const SDL_Event &event);
        void sync_mouse_state();

    private:
        // SDL resources
        SDL_Window *m_window = nullptr;

        // WebGPU resources
        WGPUInstance m_instance = nullptr;
        WGPUAdapter m_adapter = nullptr;
        WGPUDevice m_device = nullptr;
        WGPUQueue m_queue = nullptr;
        WGPUSurface m_surface = nullptr;
        WGPUTextureFormat m_surfaceFormat;

        // Depth texture for the main render pass.
        // This is owned by the App class because its lifecycle is tied to the
        // application window's size and the main render loop. It is passed to
        // the WorldRenderer, which uses it as the depth attachment.
        WGPUTexture m_depthTexture = nullptr;
        WGPUTextureView m_depthTextureView = nullptr;
        WGPUTextureFormat m_depthTextureFormat = WGPUTextureFormat_Depth24Plus;

        graphics::WorldRenderer m_worldRenderer;
        graphics::SelectionRenderer m_selectionRenderer;
        graphics::CrosshairRenderer m_crosshairRenderer;
        graphics::DebugScreenRenderer m_debugScreenRenderer;
        graphics::InventoryUIRenderer m_inventoryUIRenderer;

        Camera m_camera;
        player::Player m_player;
        GameState m_gameState;

        // App state
        bool m_running = false;
        bool m_showDebugScreen = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
        float m_initialFovY = 60.0f;

    };

} // namespace flint