#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>

#include "graphics/mesh.hpp"
#include "world.hpp"
#include "player.hpp"
#include "graphics/mesh.hpp"
#include <unordered_map>
#include <vector>

namespace flint {

    struct ChunkRenderData {
        WGPUBuffer vertex_buffer = nullptr;
        WGPUBuffer index_buffer = nullptr;
        uint32_t index_count = 0;

        void cleanup() {
            if (vertex_buffer) wgpuBufferRelease(vertex_buffer);
            if (index_buffer) wgpuBufferRelease(index_buffer);
        }
    };

    class App {
    public:
        App();
        bool Initialize(int width = 800, int height = 600);
        void Run();
        void Terminate();

    private:
        void update();
        void render();
        void load_chunks();
        void build_chunk_mesh(int chunk_x, int chunk_z);

    private:
        // SDL resources
        SDL_Window* m_window = nullptr;

        // WebGPU resources
        WGPUInstance m_instance = nullptr;
        WGPUAdapter m_adapter = nullptr;
        WGPUDevice m_device = nullptr;
        WGPUQueue m_queue = nullptr;
        WGPUSurface m_surface = nullptr;
        WGPUTextureFormat m_surfaceFormat;
        WGPURenderPipeline m_renderPipeline = nullptr;
        WGPUDepthStencilState m_depthStencilState;
        WGPUTexture m_depthTexture = nullptr;
        WGPUTextureView m_depthTextureView = nullptr;

        World m_world;
        player::Player m_player;

        WGPUBuffer m_uniformBuffer = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;
        WGPUBindGroupLayout m_bindGroupLayout = nullptr;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;

        // Rendering data
        std::unordered_map<glm::ivec2, ChunkRenderData, IVec2Hash> m_chunk_render_data;
        std::vector<glm::ivec2> m_active_chunk_coords;
    };

} // namespace flint