#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include "chunk.hpp"
#include "player.hpp"
#include "input.hpp"
#include "raycast.hpp"
#include "ui/crosshair.hpp"

namespace flint
{

    class App
    {
    public:
        App();
        bool Initialize(int width = 800, int height = 600);
        void Run();
        void Terminate();

    private:
        void render();

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
        WGPUTexture m_depth_texture = nullptr;
        WGPUTextureView m_depth_texture_view = nullptr;
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;
        WGPURenderPipeline m_renderPipeline = nullptr;

        // Scene resources
        World m_world;
        player::Player m_player;
        graphics::Texture m_texture_atlas;

        // Camera uniform resources
        WGPUBuffer m_camera_uniform_buffer = nullptr;
        WGPUBindGroup m_camera_bind_group = nullptr;
        WGPUBindGroupLayout m_camera_bind_group_layout = nullptr;

        // Texture resources
        WGPUBindGroup m_texture_bind_group = nullptr;
        WGPUBindGroupLayout m_texture_bind_group_layout = nullptr;


        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
        InputState m_input;
        std::optional<RaycastResult> m_raycast_result;
        ui::Crosshair m_crosshair;
    };

} // namespace flint