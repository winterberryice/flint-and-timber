#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#include "camera.h"
#include "chunk.h"
#include "graphics/chunk_mesh.hpp"
#include "graphics/selection_renderer.hpp"
#include "graphics/texture.hpp"
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

        WGPUTexture m_depthTexture = nullptr;
        WGPUTextureView m_depthTextureView = nullptr;
        WGPUTextureFormat m_depthTextureFormat = WGPUTextureFormat_Depth24Plus;

        // Shaders
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;
        WGPUShaderModule m_selectionVertexShader = nullptr;
        WGPUShaderModule m_selectionFragmentShader = nullptr;

        // Pipelines
        WGPURenderPipeline m_renderPipeline = nullptr;
        WGPURenderPipeline m_selectionRenderPipeline = nullptr;

        // Shared resources
        WGPUBuffer m_uniformBuffer = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;
        WGPUBindGroupLayout m_bindGroupLayout = nullptr;

        // Game objects
        Camera m_camera;
        CameraUniform m_cameraUniform;
        player::Player m_player;
        Chunk m_chunk;
        graphics::ChunkMesh m_chunkMesh;
        graphics::Texture m_atlas;
        graphics::SelectionRenderer m_selectionRenderer;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
    };

} // namespace flint