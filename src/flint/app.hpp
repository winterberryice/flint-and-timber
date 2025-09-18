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
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUShaderModule m_shaderModule = nullptr;
        WGPURenderPipeline m_renderPipeline = nullptr;

        // New texture atlas
        graphics::Texture m_blockAtlas;

        Chunk m_chunk;
        player::Player m_player;

        WGPUBuffer m_uniformBuffer = nullptr;

        // Bind groups and layouts
        WGPUBindGroup m_cameraBindGroup = nullptr;
        WGPUBindGroupLayout m_cameraBindGroupLayout = nullptr;
        WGPUBindGroup m_textureBindGroup = nullptr;
        WGPUBindGroupLayout m_textureBindGroupLayout = nullptr;


        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
    };

} // namespace flint