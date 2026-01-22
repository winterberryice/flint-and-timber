#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

namespace flint
{
    class World;
    namespace player
    {
        class Player;
    }
}

namespace flint::graphics
{

    class DebugScreenRenderer
    {
    public:
        DebugScreenRenderer();
        ~DebugScreenRenderer();

        void init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat);
        void cleanup();

        // Creates ImGui windows (does NOT manage frame lifecycle)
        void render_ui(const flint::player::Player &player, const flint::World &world);

    private:
        SDL_Window *m_window = nullptr;
        WGPUDevice m_device = nullptr;
    };

} // namespace flint::graphics
