#include "webgpu-utils.h"
#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>

class Application
{
public:
    // Initialize everything and return true if it went all right
    bool Initialize();

    // Uninitialize everything that was initialized
    void Terminate();

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    bool IsRunning();

private:
    WGPUTextureView GetNextSurfaceTextureView();

private:
    SDL_Event event;
    // We put here all the variables that are shared between init and main loop
    SDL_Window *window;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
};