#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

namespace flint::graphics
{

    class DebugScreenRenderer
    {
    public:
        DebugScreenRenderer();
        ~DebugScreenRenderer();

        void init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat);
        void process_event(const SDL_Event &event);
        void begin_frame();
        void render(WGPURenderPassEncoder renderPass);
        void cleanup();

    private:
        SDL_Window *m_window = nullptr;
        WGPUDevice m_device = nullptr;
    };

} // namespace flint::graphics
