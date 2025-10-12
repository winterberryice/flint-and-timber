#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#include "camera.h"
#include "chunk.h"
#include "camera.h"
#include "graphics/world_renderer.h"
#include "graphics/selection_renderer.h"
#include "graphics/crosshair_renderer.h"
#include "player.h"

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

        Camera m_camera;
        player::Player m_player;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
        float m_initialFovY = 45.0f;
        float m_initialAspectRatio;
        float m_initialFovX;
    };

} // namespace flint