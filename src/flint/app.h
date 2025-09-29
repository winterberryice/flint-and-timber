#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#include "camera.h"
#include "chunk.h"
#include "graphics/chunk_mesh.hpp"
#include "graphics/debug_mesh.hpp"
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

        // New depth texture fields
        WGPUTexture m_depthTexture = nullptr;
        WGPUTextureView m_depthTextureView = nullptr;
        WGPUTextureFormat m_depthTextureFormat = WGPUTextureFormat_Depth24Plus;

        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;
        WGPURenderPipeline m_renderPipeline = nullptr;

        Camera m_camera;
        CameraUniform m_cameraUniform;

        player::Player m_player;

        Chunk m_chunk;
        graphics::ChunkMesh m_chunkMesh;

        // Debugging resources
        graphics::DebugMesh m_debugMesh;
        WGPURenderPipeline m_debugRenderPipeline = nullptr;
        WGPUBuffer m_debugModelUniformBuffer = nullptr;
        WGPUBuffer m_debugColorUniformBuffer = nullptr;
        WGPUBindGroup m_debugBindGroup = nullptr;
        WGPUBindGroupLayout m_debugBindGroupLayout = nullptr;

        WGPUBuffer m_uniformBuffer = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;
        WGPUBindGroupLayout m_bindGroupLayout = nullptr;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
    };

} // namespace flint