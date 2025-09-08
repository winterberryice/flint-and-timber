#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>

namespace flint
{

    class App
    {
    public:
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
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;
        WGPURenderPipeline m_renderPipeline = nullptr;

        // App state
        bool m_running = false;
        int m_windowWidth = 800;
        int m_windowHeight = 600;
    };

} // namespace flint