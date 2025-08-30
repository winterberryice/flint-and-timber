#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <thread>

// Global WebGPU objects
WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUDevice device = nullptr;
WGPUQueue queue = nullptr;
WGPUSurface surface = nullptr;

// Global flags for async operations
volatile bool adapterRequested = false;
volatile bool adapterReceived = false;
volatile bool deviceRequested = false;
volatile bool deviceReceived = false;

// Helper function to create WGPUStringView from const char*
WGPUStringView makeStringView(const char *str)
{
    return WGPUStringView{
        .data = str,
        .length = str ? strlen(str) : 0};
}

// Callback for adapter request
void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter result, WGPUStringView message, void *userdata1, void *userdata2)
{
    std::cout << "Adapter callback called with status: " << status << std::endl;

    if (status == WGPURequestAdapterStatus_Success)
    {
        adapter = result;
        std::cout << "✓ Adapter obtained in callback!" << std::endl;

        // Get and print adapter info
        WGPUAdapterInfo adapterInfo = {};
        wgpuAdapterGetInfo(adapter, &adapterInfo);

        std::cout << "=== Adapter Info ===" << std::endl;
        if (adapterInfo.vendor.data)
        {
            std::cout << "Vendor: " << std::string(adapterInfo.vendor.data, adapterInfo.vendor.length) << std::endl;
        }
        if (adapterInfo.architecture.data)
        {
            std::cout << "Architecture: " << std::string(adapterInfo.architecture.data, adapterInfo.architecture.length) << std::endl;
        }
        if (adapterInfo.device.data)
        {
            std::cout << "Device: " << std::string(adapterInfo.device.data, adapterInfo.device.length) << std::endl;
        }
        if (adapterInfo.description.data)
        {
            std::cout << "Description: " << std::string(adapterInfo.description.data, adapterInfo.description.length) << std::endl;
        }
        std::cout << "Backend Type: " << adapterInfo.backendType << std::endl;
        std::cout << "Adapter Type: " << adapterInfo.adapterType << std::endl;
    }
    else
    {
        std::cerr << "✗ Adapter request failed with status: " << status << std::endl;
        if (message.data && message.length > 0)
        {
            std::cerr << "Error message: " << std::string(message.data, message.length) << std::endl;
        }
    }

    adapterReceived = true;
}

// Callback for device request
void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice result, WGPUStringView message, void *userdata1, void *userdata2)
{
    std::cout << "Device callback called with status: " << status << std::endl;

    if (status == WGPURequestDeviceStatus_Success)
    {
        device = result;
        std::cout << "✓ Device obtained in callback!" << std::endl;
    }
    else
    {
        std::cerr << "✗ Device request failed with status: " << status << std::endl;
        if (message.data && message.length > 0)
        {
            std::cerr << "Error message: " << std::string(message.data, message.length) << std::endl;
        }
    }

    deviceReceived = true;
}

bool waitForAdapter(int timeoutMs = 5000)
{
    auto start = std::chrono::steady_clock::now();

    while (!adapterReceived)
    {
        // Process events to allow callbacks to be called
        if (instance)
        {
            wgpuInstanceProcessEvents(instance);
        }

        // Check timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (elapsed > timeoutMs)
        {
            std::cout << "Adapter request timed out after " << timeoutMs << "ms" << std::endl;
            return false;
        }

        // Small delay to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return adapter != nullptr;
}

bool waitForDevice(int timeoutMs = 5000)
{
    auto start = std::chrono::steady_clock::now();

    while (!deviceReceived)
    {
        // Process events to allow callbacks to be called
        if (instance)
        {
            wgpuInstanceProcessEvents(instance);
        }

        // Check timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (elapsed > timeoutMs)
        {
            std::cout << "Device request timed out after " << timeoutMs << "ms" << std::endl;
            return false;
        }

        // Small delay to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return device != nullptr;
}

bool initWebGPU(SDL_Window *window)
{
    std::cout << "\n=== Initializing WebGPU ===" << std::endl;

    // Create WebGPU instance
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;

    instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        return false;
    }

    std::cout << "✓ WebGPU instance created successfully" << std::endl;

    // Create surface from SDL window
    surface = SDL_GetWGPUSurface(instance, window);
    if (!surface)
    {
        std::cerr << "Failed to create WebGPU surface!" << std::endl;
        return false;
    }

    std::cout << "✓ WebGPU surface created successfully" << std::endl;

    // Request adapter
    std::cout << "Requesting WebGPU adapter..." << std::endl;

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapterOpts.backendType = WGPUBackendType_Undefined;
    adapterOpts.forceFallbackAdapter = false;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = nullptr;
    callbackInfo.userdata2 = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

    adapterRequested = true;
    adapterReceived = false;

    WGPUFuture adapterFuture = wgpuInstanceRequestAdapter(instance, &adapterOpts, callbackInfo);
    std::cout << "Adapter request submitted, waiting for callback..." << std::endl;

    bool adapterSuccess = waitForAdapter();

    if (!adapterSuccess || !adapter)
    {
        std::cerr << "Failed to get WebGPU adapter!" << std::endl;
        return false;
    }

    std::cout << "✓ Got WebGPU adapter successfully" << std::endl;

    // Request device
    std::cout << "\nRequesting WebGPU device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = makeStringView("Main Device");

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = onDeviceRequestEnded;
    deviceCallbackInfo.userdata1 = nullptr;
    deviceCallbackInfo.userdata2 = nullptr;
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

    deviceRequested = true;
    deviceReceived = false;

    WGPUFuture deviceFuture = wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);
    std::cout << "Device request submitted, waiting for callback..." << std::endl;

    bool deviceSuccess = waitForDevice();

    if (!deviceSuccess || !device)
    {
        std::cerr << "Failed to get WebGPU device!" << std::endl;
        return false;
    }

    // Get queue
    queue = wgpuDeviceGetQueue(device);
    if (!queue)
    {
        std::cerr << "Failed to get WebGPU queue!" << std::endl;
        return false;
    }

    std::cout << "✓ WebGPU queue obtained successfully" << std::endl;

    // Configure surface
    WGPUSurfaceCapabilities surfaceCaps = {};
    wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCaps);

    if (surfaceCaps.formatCount == 0)
    {
        std::cerr << "No surface formats available!" << std::endl;
        return false;
    }

    std::cout << "Available surface formats: " << surfaceCaps.formatCount << std::endl;

    WGPUSurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.device = device;
    surfaceConfig.format = surfaceCaps.formats[0]; // Use first available format
    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfig.width = 800;
    surfaceConfig.height = 600;
    surfaceConfig.presentMode = WGPUPresentMode_Fifo;
    surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &surfaceConfig);
    std::cout << "✓ Surface configured successfully" << std::endl;

    std::cout << "=== WebGPU initialization complete! ===" << std::endl;
    return true;
}

void render()
{
    if (!surface || !device || !queue)
        return;

    // Get next surface texture
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

    if (!surfaceTexture.texture)
    {
        return;
    }

    // Create command encoder
    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = makeStringView("Command Encoder");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

    // Create render pass
    WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = textureView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.5, 0.0, 0.5, 1.0}; // Purple background

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.label = makeStringView("Render Pass");
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    wgpuRenderPassEncoderEnd(renderPass);

    // Submit commands
    WGPUCommandBufferDescriptor commandBufferDesc = {};
    commandBufferDesc.nextInChain = nullptr;
    commandBufferDesc.label = makeStringView("Command Buffer");

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
    wgpuQueueSubmit(queue, 1, &commandBuffer);

    // Present
    wgpuSurfacePresent(surface);

    // Cleanup frame resources immediately
    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(encoder);
    wgpuRenderPassEncoderRelease(renderPass);
    wgpuTextureViewRelease(textureView);
    wgpuTextureRelease(surfaceTexture.texture);
}

void cleanup()
{
    std::cout << "Cleaning up WebGPU resources..." << std::endl;

    try
    {
        // Wait for any pending operations to complete
        if (queue)
        {
            // Process any remaining events
            if (instance)
            {
                wgpuInstanceProcessEvents(instance);
            }

            // Wait for idle (if function exists)
#ifdef WGPU_QUEUE_ON_SUBMITTED_WORK_DONE_CALLBACK_INFO
            // This might not exist in this Dawn version
#endif
        }

        // Release in reverse order of creation
        if (queue)
        {
            wgpuQueueRelease(queue);
            queue = nullptr;
            std::cout << "✓ Queue released" << std::endl;
        }

        if (device)
        {
            wgpuDeviceRelease(device);
            device = nullptr;
            std::cout << "✓ Device released" << std::endl;
        }

        if (adapter)
        {
            wgpuAdapterRelease(adapter);
            adapter = nullptr;
            std::cout << "✓ Adapter released" << std::endl;
        }

        if (surface)
        {
            wgpuSurfaceRelease(surface);
            surface = nullptr;
            std::cout << "✓ Surface released" << std::endl;
        }

        if (instance)
        {
            // Final event processing before releasing instance
            wgpuInstanceProcessEvents(instance);
            wgpuInstanceRelease(instance);
            instance = nullptr;
            std::cout << "✓ Instance released" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception during cleanup: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception during cleanup" << std::endl;
    }

    std::cout << "Cleanup completed" << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "Starting SDL3 + WebGPU (Dawn 7187) application" << std::endl;

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

    std::cout << "SDL window created successfully" << std::endl;

    // Initialize WebGPU
    if (!initWebGPU(window))
    {
        std::cout << "WebGPU initialization failed" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    std::cout << "\n🎉 SUCCESS! Window should show purple background rendered by WebGPU!" << std::endl;
    std::cout << "Press ESC or close window to exit." << std::endl;

    // Render a few frames to make sure it's really working
    int frameCount = 0;
    while (running)
    {
        // Handle events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                std::cout << "Quit requested" << std::endl;
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE)
                {
                    std::cout << "Escape pressed" << std::endl;
                    running = false;
                }
                break;
            }
        }

        // Render with WebGPU
        render();
        frameCount++;

        if (frameCount == 10)
        {
            std::cout << "Rendered 10 frames successfully!" << std::endl;
        }

        // Process WebGPU events
        if (instance)
        {
            wgpuInstanceProcessEvents(instance);
        }

        // Small delay
        SDL_Delay(16); // ~60 FPS
    }

    // Proper cleanup sequence
    std::cout << "\nExiting main loop..." << std::endl;
    cleanup();

    std::cout << "Destroying SDL window..." << std::endl;
    SDL_DestroyWindow(window);

    std::cout << "Quitting SDL..." << std::endl;
    SDL_Quit();

    std::cout << "Application exited successfully!" << std::endl;
    return 0;
}
