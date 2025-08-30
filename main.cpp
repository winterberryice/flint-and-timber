#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>
#include <iostream>
#include <cassert>
#include <cstring>

// Global WebGPU objects
WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUDevice device = nullptr;
WGPUQueue queue = nullptr;
WGPUSurface surface = nullptr;

// Helper function to create WGPUStringView from const char*
WGPUStringView makeStringView(const char *str)
{
    return WGPUStringView{
        .data = str,
        .length = str ? strlen(str) : 0};
}

// Updated callback signatures for Dawn 7187
void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter result, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        adapter = result;
    }
    else
    {
        std::cerr << "Failed to get WebGPU adapter: ";
        if (message.data)
        {
            std::cerr << std::string(message.data, message.length);
        }
        std::cerr << std::endl;
    }
}

void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice result, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        device = result;
    }
    else
    {
        std::cerr << "Failed to get WebGPU device: ";
        if (message.data)
        {
            std::cerr << std::string(message.data, message.length);
        }
        std::cerr << std::endl;
    }
}

bool initWebGPU(SDL_Window *window)
{
    // Create WebGPU instance
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;

    instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        return false;
    }

    std::cout << "WebGPU instance created successfully" << std::endl;

    // Create surface from SDL window
    surface = SDL_GetWGPUSurface(instance, window);
    if (!surface)
    {
        std::cerr << "Failed to create WebGPU surface!" << std::endl;
        return false;
    }

    std::cout << "WebGPU surface created successfully" << std::endl;

    // Request adapter
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = nullptr;
    callbackInfo.userdata2 = nullptr;

    wgpuInstanceRequestAdapter(instance, &adapterOpts, callbackInfo);

    // Process events to handle the callback
    wgpuInstanceProcessEvents(instance);

    if (!adapter)
    {
        std::cerr << "Failed to get WebGPU adapter!" << std::endl;
        return false;
    }

    std::cout << "WebGPU adapter acquired successfully" << std::endl;

    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = makeStringView("Primary Device");
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = makeStringView("Primary Queue");

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.nextInChain = nullptr;
    deviceCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    deviceCallbackInfo.callback = onDeviceRequestEnded;
    deviceCallbackInfo.userdata1 = nullptr;
    deviceCallbackInfo.userdata2 = nullptr;

    wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);

    // Process events to handle the callback
    wgpuInstanceProcessEvents(instance);

    if (!device)
    {
        std::cerr << "Failed to get WebGPU device!" << std::endl;
        return false;
    }

    // Skip error callback setup since it's not available in this Dawn version
    std::cout << "WebGPU device acquired successfully (error callback not available in this Dawn version)" << std::endl;

    // Get queue
    queue = wgpuDeviceGetQueue(device);
    if (!queue)
    {
        std::cerr << "Failed to get WebGPU queue!" << std::endl;
        return false;
    }

    std::cout << "WebGPU device and queue acquired successfully" << std::endl;

    // Configure surface
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.device = device;
    config.format = WGPUTextureFormat_BGRA8Unorm;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = 800;
    config.height = 600;
    config.presentMode = WGPUPresentMode_Fifo;

    wgpuSurfaceConfigure(surface, &config);

    std::cout << "WebGPU surface configured successfully" << std::endl;
    return true;
}

void render()
{
    // Get current surface texture
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

    // Simple status check - if we got a texture, proceed
    if (!surfaceTexture.texture)
    {
        std::cerr << "Failed to get current surface texture!" << std::endl;
        return;
    }

    // Create texture view
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.nextInChain = nullptr;
    viewDesc.format = WGPUTextureFormat_BGRA8Unorm;
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = WGPUTextureAspect_All;

    WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);

    // Create command encoder
    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = makeStringView("Command Encoder");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

    // Create render pass
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.nextInChain = nullptr;
    colorAttachment.view = view;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.3, 0.1, 0.4, 1.0}; // Purple color

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.label = makeStringView("Render Pass");
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    wgpuRenderPassEncoderEnd(renderPass);

    // Create command buffer
    WGPUCommandBufferDescriptor commandBufferDesc = {};
    commandBufferDesc.nextInChain = nullptr;
    commandBufferDesc.label = makeStringView("Command Buffer");

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);

    // Submit command buffer
    wgpuQueueSubmit(queue, 1, &commandBuffer);

    // Present surface
    wgpuSurfacePresent(surface);

    // Cleanup
    wgpuRenderPassEncoderRelease(renderPass);
    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(surfaceTexture.texture);
}

void cleanup()
{
    if (queue)
    {
        wgpuQueueRelease(queue);
        queue = nullptr;
    }
    if (surface)
    {
        wgpuSurfaceRelease(surface);
        surface = nullptr;
    }
    if (device)
    {
        wgpuDeviceRelease(device);
        device = nullptr;
    }
    if (adapter)
    {
        wgpuAdapterRelease(adapter);
        adapter = nullptr;
    }
    if (instance)
    {
        wgpuInstanceRelease(instance);
        instance = nullptr;
    }
}

int main(int argc, char *argv[])
{
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "SDL3 + WebGPU (Dawn 7187)",
        800, 600,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Initialize WebGPU
    if (!initWebGPU(window))
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    std::cout << "SDL3 + WebGPU window created successfully!" << std::endl;
    std::cout << "You should see a purple screen rendered by WebGPU!" << std::endl;
    std::cout << "Press ESC or close window to exit." << std::endl;

    while (running)
    {
        // Handle events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE)
                {
                    running = false;
                }
                break;
            }
        }

        // Render with WebGPU
        render();

        // Small delay
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Cleanup complete!" << std::endl;
    return 0;
}
