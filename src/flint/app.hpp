#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>

#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include "chunk.hpp"
#include "player.hpp"

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
        WGPURenderPipeline m_renderPipeline = nullptr;

        graphics::Texture m_blockAtlas;
        Chunk m_chunk;
        player::Player m_player;

        // Uniforms (Group 0)
        WGPUBuffer m_uniformBuffer = nullptr;
        WGPUBindGroup m_uniformBindGroup = nullptr;
        WGPUBindGroupLayout m_uniformBindGroupLayout = nullptr;

        // Textures (Group 1)
        WGPUBindGroup m_textureBindGroup = nullptr;
        WGPUBindGroupLayout m_textureBindGroupLayout = nullptr;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
    };

} // namespace flint