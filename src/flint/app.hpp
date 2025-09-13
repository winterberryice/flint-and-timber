#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>
#include <optional>
#include <memory>
#include "player.hpp"
#include "world.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "ui/crosshair.hpp"
#include "ui/inventory.hpp"
#include "ui/hotbar.hpp"
#include "ui/item_renderer.hpp"
#include "ui/ui_text.hpp"
#include "wireframe_renderer.hpp"
#include "raycast.hpp"
#include <map>
#include <vector>
#include <utility>

namespace flint
{
    class App
    {
    public:
        App();
        bool Initialize(int width = 1280, int height = 720);
        void Run();
        void Terminate();

    private:
        void render();
        void handle_event(const SDL_Event &event);
        void set_mouse_grab(bool grab);

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
        WGPUBuffer m_camera_buffer = nullptr;

        // App state
        bool m_running = false;
        int m_windowWidth = 1280;
        int m_windowHeight = 720;
        bool m_mouse_grabbed = false;

        // Game state
        World m_world;
        Player m_player;
        CameraUniform m_camera_uniform;
        Input m_input;

        // UI components
        ui::Crosshair m_crosshair;
        ui::Inventory m_inventory;
        ui::Hotbar m_hotbar;
        ui::ItemRenderer m_item_renderer;
        ui::UiText m_ui_text;

        // Renderers
        WireframeRenderer m_wireframe_renderer;

        // Raycasting
        Raycast m_raycast;
    };

} // namespace flint
