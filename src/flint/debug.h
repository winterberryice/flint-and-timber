#pragma once

#include "player.h"
#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

namespace flint
{
    void printVideoSystemInfo();
    void printDetailedVideoInfo(SDL_Window *window);

    class DebugBox
    {
    public:
        DebugBox();
        ~DebugBox();

        void create(WGPUDevice device, const flint::player::Player &player);
        void update(const flint::player::Player &player);
        void render(WGPURenderPassEncoder renderPass);
        void cleanup();

    private:
        WGPUDevice m_device = nullptr;
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        uint32_t m_indexCount = 0;
    };
}