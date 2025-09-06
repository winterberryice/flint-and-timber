#include "flint/app.hpp"
#include <iostream>

namespace
{
    // Helper for synchronous adapter request
    struct AdapterRequestData
    {
        WGPUAdapter adapter = nullptr;
        bool done = false;
    };

    static void OnAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
    {
        std::cout << "OnAdapterReceived called with status: " << status << std::endl;

        AdapterRequestData *data = static_cast<AdapterRequestData *>(userdata1);
        if (status == WGPURequestAdapterStatus_Success)
        {
            std::cout << "Adapter request successful" << std::endl;
            data->adapter = adapter;
        }
        else
        {
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get adapter: " << errorMsg << " (status: " << status << ")" << std::endl;
        }
        data->done = true;
        std::cout << "Adapter callback completed. Done: " << data->done << std::endl;
    }

    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    static void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
    {
        std::cout << "OnDeviceReceived called with status: " << status << std::endl;

        DeviceRequestData *data = static_cast<DeviceRequestData *>(userdata1);
        if (status == WGPURequestDeviceStatus_Success)
        {
            std::cout << "Device request successful" << std::endl;
            data->device = device;
        }
        else
        {
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get device: " << errorMsg << " (status: " << status << ")" << std::endl;
        }
        data->done = true;
        std::cout << "Device callback completed. Done: " << data->done << std::endl;
    }

    // Helper function to create WGPUStringView from const char*
    WGPUStringView makeStringView(const char *str)
    {
        return WGPUStringView{
            .data = str,
            .length = str ? strlen(str) : 0};
    }

} // namespace

namespace flint
{
    bool App::Initialize(int windowWidth, int windowHeight)
    {
        std::cout << "Initializing app..." << std::endl;

        m_windowWidth = windowWidth;
        m_windowHeight = windowHeight;

        // Initialize SDL
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        std::cout << "SDL initialized successfully" << std::endl;

        // Set SDL hints for WebGPU compatibility
        SDL_SetHint("SDL_VIDEO_EXTERNAL_CONTEXT", "1");

        // Create window - SDL3 syntax (windows are shown by default)
        m_window = SDL_CreateWindow("Flint and Timber",
                                    m_windowWidth, m_windowHeight,
                                    SDL_WINDOW_RESIZABLE);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }
        std::cout << "SDL window created successfully" << std::endl;

        // Create WebGPU instance
        WGPUInstanceDescriptor instanceDesc = {};
        instanceDesc.nextInChain = nullptr;

        m_instance = wgpuCreateInstance(&instanceDesc);
        if (!m_instance)
        {
            std::cerr << "Failed to create WebGPU instance" << std::endl;
            return false;
        }
        std::cout << "WebGPU instance created" << std::endl;

        // Create surface FIRST
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);
        if (!m_surface)
        {
            std::cerr << "Failed to create WebGPU surface" << std::endl;
            return false;
        }
        std::cout << "WebGPU surface created successfully" << std::endl;

        // Request adapter with surface compatibility
        std::cout << "Requesting WebGPU adapter..." << std::endl;

        AdapterRequestData adapterData = {};

        WGPURequestAdapterOptions adapterOpts = {};
        adapterOpts.nextInChain = nullptr;
        adapterOpts.compatibleSurface = m_surface; // CRITICAL: Pass surface
        adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
        adapterOpts.backendType = WGPUBackendType_Undefined;
        adapterOpts.forceFallbackAdapter = false;

        WGPURequestAdapterCallbackInfo callbackInfo = {};
        callbackInfo.callback = OnAdapterReceived;
        callbackInfo.userdata1 = &adapterData;
        callbackInfo.userdata2 = nullptr;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

        wgpuInstanceRequestAdapter(m_instance, &adapterOpts, callbackInfo);

        // Process events until adapter callback completes
        std::cout << "Processing events until adapter callback..." << std::endl;
        while (!adapterData.done)
        {
            wgpuInstanceProcessEvents(m_instance);
            SDL_Delay(1);
        }

        if (!adapterData.adapter)
        {
            std::cerr << "Failed to get WebGPU adapter" << std::endl;
            return false;
        }

        m_adapter = adapterData.adapter;
        std::cout << "WebGPU adapter obtained successfully" << std::endl;

        // Request device
        std::cout << "Requesting WebGPU device..." << std::endl;

        DeviceRequestData deviceData = {};

        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain = nullptr;
        deviceDesc.label = makeStringView("Main Device");

        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;
        deviceCallbackInfo.userdata2 = nullptr;
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;

        wgpuAdapterRequestDevice(m_adapter, &deviceDesc, deviceCallbackInfo);

        // Process events until device callback completes
        std::cout << "Processing events until device callback..." << std::endl;
        while (!deviceData.done)
        {
            wgpuInstanceProcessEvents(m_instance);
            SDL_Delay(1);
        }

        if (!deviceData.device)
        {
            std::cerr << "Failed to get WebGPU device" << std::endl;
            return false;
        }

        m_device = deviceData.device;
        std::cout << "WebGPU device created successfully" << std::endl;

        // Get queue
        m_queue = wgpuDeviceGetQueue(m_device);
        if (!m_queue)
        {
            std::cerr << "Failed to get WebGPU queue" << std::endl;
            return false;
        }
        std::cout << "WebGPU queue obtained" << std::endl;

        // Configure surface
        WGPUSurfaceCapabilities surfaceCaps = {};
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &surfaceCaps);

        if (surfaceCaps.formatCount == 0)
        {
            std::cerr << "No surface formats available!" << std::endl;
            return false;
        }

        std::cout << "Supported surface formats: ";
        for (uint32_t i = 0; i < surfaceCaps.formatCount; ++i)
        {
            std::cout << surfaceCaps.formats[i] << " ";
        }
        std::cout << std::endl;

        WGPUTextureFormat preferredFormat = surfaceCaps.formats[0];
        std::cout << "Using format: " << preferredFormat;

        // Add format name for clarity
        switch (preferredFormat)
        {
        case WGPUTextureFormat_BGRA8Unorm:
            std::cout << " (BGRA8Unorm)";
            break;
        case WGPUTextureFormat_RGBA8Unorm:
            std::cout << " (RGBA8Unorm)";
            break;
        default:
            std::cout << " (Other)";
            break;
        }
        std::cout << std::endl;

        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.device = m_device;
        surfaceConfig.format = preferredFormat;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = static_cast<uint32_t>(m_windowWidth);
        surfaceConfig.height = static_cast<uint32_t>(m_windowHeight);
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        std::cout << "WebGPU surface configured" << std::endl;

        m_running = true;
        return true;
    }

    void App::Run()
    {
        std::cout << "Running app..." << std::endl;
        std::cout << "Window should now be visible" << std::endl;

        SDL_Event event;
        int frameCount = 0;

        while (m_running)
        {
            // Handle SDL events
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    std::cout << "Quit event received" << std::endl;
                    m_running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        std::cout << "Escape key pressed" << std::endl;
                        m_running = false;
                    }
                    break;
                }
            }

            frameCount++;

            // Get surface texture
            WGPUSurfaceTexture surfaceTexture;
            wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

            if (!surfaceTexture.texture || (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal &&
                                            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal))
            {
                if (frameCount <= 5)
                {
                    std::cerr << "Failed to get surface texture, status: " << surfaceTexture.status << std::endl;
                }
                continue;
            }

            if (frameCount == 1)
            {
                std::cout << "Successfully got surface texture on first frame" << std::endl;
            }

            // Create texture view
            WGPUTextureViewDescriptor textureViewDesc = {};
            textureViewDesc.nextInChain = nullptr;
            textureViewDesc.label = makeStringView("Surface texture view");
            textureViewDesc.format = WGPUTextureFormat_Undefined; // Use surface format
            textureViewDesc.dimension = WGPUTextureViewDimension_2D;
            textureViewDesc.baseMipLevel = 0;
            textureViewDesc.mipLevelCount = 1;
            textureViewDesc.baseArrayLayer = 0;
            textureViewDesc.arrayLayerCount = 1;
            textureViewDesc.aspect = WGPUTextureAspect_All;

            WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, &textureViewDesc);
            if (!textureView)
            {
                std::cerr << "Failed to create texture view!" << std::endl;
                continue;
            }

            // Create command encoder
            WGPUCommandEncoderDescriptor encoderDesc = {};
            encoderDesc.nextInChain = nullptr;
            encoderDesc.label = makeStringView("Command Encoder");

            WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

            // CRITICAL FIX: Use correct color attachment structure
            WGPURenderPassColorAttachment colorAttachment = {};
            colorAttachment.nextInChain = nullptr;
            colorAttachment.view = textureView;
            colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED; // CRITICAL: Add this
            colorAttachment.resolveTarget = nullptr;
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            colorAttachment.clearValue = {0.8f, 0.2f, 0.8f, 1.0f}; // Bright purple like working example

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.nextInChain = nullptr;
            renderPassDesc.label = makeStringView("Render Pass");
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = nullptr;
            renderPassDesc.timestampWrites = nullptr; // CRITICAL: Add this

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
            if (!renderPass)
            {
                std::cerr << "Failed to create render pass!" << std::endl;
                continue;
            }

            wgpuRenderPassEncoderEnd(renderPass);

            WGPUCommandBufferDescriptor cmdBufferDesc = {};
            cmdBufferDesc.nextInChain = nullptr;
            cmdBufferDesc.label = makeStringView("Command Buffer");

            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
            wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

            wgpuSurfacePresent(m_surface);

            if (frameCount == 1)
            {
                std::cout << "First frame rendered and presented" << std::endl;
            }

            if (frameCount > 0 && frameCount % 60 == 0)
            {
                std::cout << "Frame " << frameCount << std::endl;
            }

            // Cleanup
            wgpuCommandBufferRelease(cmdBuffer);
            wgpuRenderPassEncoderRelease(renderPass);
            wgpuCommandEncoderRelease(encoder);
            wgpuTextureViewRelease(textureView);
            // Don't release surfaceTexture.texture - it's managed by the surface

            // Process WebGPU events
            wgpuInstanceProcessEvents(m_instance);

            // Small delay to prevent 100% CPU usage
            SDL_Delay(16); // ~60 FPS
        }

        std::cout << "Render loop ended after " << frameCount << " frames" << std::endl;
    }

    void App::Terminate()
    {
        std::cout << "Terminating app..." << std::endl;

        // Force all pending GPU work to complete
        if (m_device && m_queue)
        {
            // Wait for device to be idle
            for (int i = 0; i < 30; ++i)
            { // Wait up to ~300ms
                wgpuInstanceProcessEvents(m_instance);
                SDL_Delay(10);
            }
        }

        // Release in very specific order with safety checks
        if (m_queue)
        {
            wgpuQueueRelease(m_queue);
            m_queue = nullptr;
        }

        if (m_surface)
        {
            wgpuSurfaceRelease(m_surface);
            m_surface = nullptr;
        }

        if (m_device)
        {
            wgpuDeviceRelease(m_device);
            m_device = nullptr;
        }

        if (m_adapter)
        {
            wgpuAdapterRelease(m_adapter);
            m_adapter = nullptr;
        }

        // Process any final events
        if (m_instance)
        {
            for (int i = 0; i < 10; ++i)
            {
                wgpuInstanceProcessEvents(m_instance);
                SDL_Delay(5);
            }
            wgpuInstanceRelease(m_instance);
            m_instance = nullptr;
        }

        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
        m_running = false;
    }

} // namespace flint
