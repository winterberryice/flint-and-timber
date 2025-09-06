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
    }

    // Add this struct after AdapterRequestData
    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    // Add this callback function
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
    }
}

const char *getFormatName(WGPUTextureFormat format)
{
    switch (format)
    {
    case 18:
        return "RGBA8Unorm";
    case 19:
        return "RGBA8UnormSrgb";
    case 23:
        return "BGRA8Unorm";
    case 24:
        return "BGRA8UnormSrgb";
    case 26:
        return "RGB10A2Unorm";
    case 34:
        return "RGBA16Float";
    default:
    {
        static char buffer[32];
        snprintf(buffer, sizeof(buffer), "Format_%d", (int)format);
        return buffer;
    }
    }
}

namespace flint
{

    App::App()
    {
        // Constructor
    }

    App::~App()
    {
        // Destructor - cleanup will be in Terminate()
    }

    bool App::Initialize(int width, int height)
    {
        std::cout << "Initializing app..." << std::endl;

        m_windowWidth = width;
        m_windowHeight = height;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create window - SDL3 syntax (windows are shown by default)
        m_window = SDL_CreateWindow("Flint and Timber",
                                    m_windowWidth, m_windowHeight,
                                    SDL_WINDOW_RESIZABLE);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }

        std::cout << "SDL initialized successfully" << std::endl;

        // Initialize WebGPU instance
        m_instance = wgpuCreateInstance(nullptr);
        if (!m_instance)
        {
            std::cerr << "Failed to create WebGPU instance" << std::endl;
            return false;
        }

        std::cout << "WebGPU instance created" << std::endl;

        // Create surface BEFORE requesting adapter - this is important!
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);
        if (!m_surface)
        {
            std::cerr << "Failed to create WebGPU surface" << std::endl;
            return false;
        }
        std::cout << "WebGPU surface created successfully" << std::endl;

        // Request adapter WITH the surface
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.compatibleSurface = m_surface; // IMPORTANT: Pass the surface here!
        adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
        adapterOptions.backendType = WGPUBackendType_Undefined;
        adapterOptions.forceFallbackAdapter = false;

        AdapterRequestData adapterData;

        WGPURequestAdapterCallbackInfo callbackInfo = {};
        callbackInfo.nextInChain = nullptr;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.callback = OnAdapterReceived;
        callbackInfo.userdata1 = &adapterData;
        callbackInfo.userdata2 = nullptr;

        std::cout << "Requesting WebGPU adapter..." << std::endl;
        wgpuInstanceRequestAdapter(m_instance, &adapterOptions, callbackInfo);

        std::cout << "Processing events until adapter callback..." << std::endl;
        // Process events until callback is called
        int attempts = 0;
        while (!adapterData.done && attempts < 1000)
        {
            wgpuInstanceProcessEvents(m_instance);
            attempts++;
            if (attempts % 100 == 0)
            {
                std::cout << "Still waiting... attempt " << attempts << std::endl;
            }
        }

        std::cout << "Adapter callback completed. Done: " << adapterData.done << std::endl;

        if (!adapterData.adapter)
        {
            std::cerr << "No suitable WebGPU adapter found" << std::endl;
            return false;
        }

        m_adapter = adapterData.adapter;
        std::cout << "WebGPU adapter obtained successfully" << std::endl;

        // Request device
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain = nullptr;
        deviceDesc.label = {nullptr, 0};
        deviceDesc.requiredFeatureCount = 0;
        deviceDesc.requiredFeatures = nullptr;
        deviceDesc.requiredLimits = nullptr;
        deviceDesc.defaultQueue.nextInChain = nullptr;
        deviceDesc.defaultQueue.label = {nullptr, 0};

        DeviceRequestData deviceData;

        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.nextInChain = nullptr;
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;
        deviceCallbackInfo.userdata2 = nullptr;

        std::cout << "Requesting WebGPU device..." << std::endl;
        wgpuAdapterRequestDevice(m_adapter, &deviceDesc, deviceCallbackInfo);

        std::cout << "Processing events until device callback..." << std::endl;
        attempts = 0;
        while (!deviceData.done && attempts < 1000)
        {
            wgpuInstanceProcessEvents(m_instance);
            attempts++;
            if (attempts % 100 == 0)
            {
                std::cout << "Still waiting for device... attempt " << attempts << std::endl;
            }
        }

        std::cout << "Device callback completed. Done: " << deviceData.done << std::endl;

        if (!deviceData.device)
        {
            std::cerr << "Failed to create WebGPU device" << std::endl;
            return false;
        }

        m_device = deviceData.device;
        std::cout << "WebGPU device created successfully" << std::endl;

        // Get the queue
        m_queue = wgpuDeviceGetQueue(m_device);
        std::cout << "WebGPU queue obtained" << std::endl;

        // Get surface capabilities
        WGPUSurfaceCapabilities caps = {};
        caps.nextInChain = nullptr;

        WGPUStatus status = wgpuSurfaceGetCapabilities(m_surface, m_adapter, &caps);
        if (status != WGPUStatus_Success)
        {
            std::cerr << "Failed to get surface capabilities" << std::endl;
            return false;
        }

        std::cout << "Supported surface formats: ";
        for (size_t i = 0; i < caps.formatCount; i++)
        {
            std::cout << caps.formats[i] << " ";
        }
        std::cout << std::endl;

        // Choose format
        WGPUTextureFormat preferredFormat = WGPUTextureFormat_Undefined;
        for (size_t i = 0; i < caps.formatCount; i++)
        {
            if (caps.formats[i] == 23) // BGRA8Unorm
            {
                preferredFormat = caps.formats[i];
                break;
            }
            else if (caps.formats[i] == 18) // RGBA8Unorm
            {
                preferredFormat = caps.formats[i];
            }
        }

        if (preferredFormat == WGPUTextureFormat_Undefined)
        {
            preferredFormat = caps.formats[0];
        }

        std::cout << "Using format: " << preferredFormat << " (" << getFormatName(preferredFormat) << ")" << std::endl;
        m_surfaceFormat = preferredFormat;

        // Configure surface
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.device = m_device;
        surfaceConfig.format = preferredFormat;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = caps.presentModes[0];
        surfaceConfig.alphaMode = caps.alphaModes[0];
        surfaceConfig.viewFormatCount = 0;
        surfaceConfig.viewFormats = nullptr;

        wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        std::cout << "WebGPU surface configured" << std::endl;

        m_running = true;
        return true;
    }

    void App::Run()
    {
        std::cout << "Running app..." << std::endl;
        std::cout << "Window should now be visible" << std::endl;

        SDL_Event e;
        int frameCount = 0;

        while (m_running)
        {
            frameCount++;
            if (frameCount % 60 == 0)
            {
                std::cout << "Frame " << frameCount << std::endl;
            }

            // Handle events
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_EVENT_QUIT)
                {
                    std::cout << "Quit event received" << std::endl;
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    std::cout << "Escape key pressed" << std::endl;
                    m_running = false;
                }
            }

            // Get surface texture
            WGPUSurfaceTexture surfaceTexture;
            wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

            if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
            {
                std::cerr << "Surface texture status: " << surfaceTexture.status << std::endl;
                if (surfaceTexture.texture)
                {
                    wgpuTextureRelease(surfaceTexture.texture);
                }
                continue;
            }

            if (!surfaceTexture.texture)
            {
                std::cerr << "Surface texture is null!" << std::endl;
                continue;
            }

            if (frameCount == 1)
            {
                std::cout << "Successfully got surface texture on first frame" << std::endl;
            }

            // Create texture view
            WGPUTextureViewDescriptor viewDesc = {};
            viewDesc.nextInChain = nullptr;
            viewDesc.format = m_surfaceFormat;
            viewDesc.dimension = WGPUTextureViewDimension_2D;
            viewDesc.baseMipLevel = 0;
            viewDesc.mipLevelCount = 1;
            viewDesc.baseArrayLayer = 0;
            viewDesc.arrayLayerCount = 1;
            viewDesc.aspect = WGPUTextureAspect_All;

            WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);
            if (!textureView)
            {
                std::cerr << "Failed to create texture view!" << std::endl;
                wgpuTextureRelease(surfaceTexture.texture);
                continue;
            }

            // Create command encoder
            WGPUCommandEncoderDescriptor encoderDesc = {};
            encoderDesc.nextInChain = nullptr;
            encoderDesc.label = {nullptr, 0};

            WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

            // Create render pass
            WGPURenderPassColorAttachment colorAttachment = {};
            colorAttachment.view = textureView;
            colorAttachment.resolveTarget = nullptr;
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            colorAttachment.clearValue = {0.0, 1.0, 0.0, 1.0}; // Bright green

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.nextInChain = nullptr;
            renderPassDesc.label = {nullptr, 0};
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = nullptr;

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
            if (!renderPass)
            {
                std::cerr << "Failed to create render pass!" << std::endl;
                wgpuCommandEncoderRelease(encoder);
                wgpuTextureViewRelease(textureView);
                wgpuTextureRelease(surfaceTexture.texture);
                continue;
            }
            // Add this right after getting the surface texture:
            std::cout << "Frame " << frameCount << " - Surface texture: "
                      << (surfaceTexture.texture ? "valid" : "null") << std::endl;

            // Add this right after creating the texture view:
            std::cout << "Frame " << frameCount << " - Texture view: "
                      << (textureView ? "valid" : "null") << std::endl;

            // Add this right after creating the render pass:
            std::cout << "Frame " << frameCount << " - Render pass: "
                      << (renderPass ? "valid" : "null") << std::endl;

            // Add this right after wgpuQueueSubmit:
            std::cout << "Frame " << frameCount << " - Queue submit completed" << std::endl;

            // Add this right after wgpuSurfacePresent:
            std::cout << "Frame " << frameCount << " - Surface present completed" << std::endl;

            // End render pass
            wgpuRenderPassEncoderEnd(renderPass);

            // Finish command buffer
            WGPUCommandBufferDescriptor cmdBufferDesc = {};
            cmdBufferDesc.nextInChain = nullptr;
            cmdBufferDesc.label = {nullptr, 0};

            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);

            // Submit commands
            wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

            // Present
            wgpuSurfacePresent(m_surface);

            if (frameCount == 1)
            {
                std::cout << "First frame rendered and presented" << std::endl;
            }

            // Release resources
            wgpuCommandBufferRelease(cmdBuffer);
            wgpuRenderPassEncoderRelease(renderPass);
            wgpuCommandEncoderRelease(encoder);
            wgpuTextureViewRelease(textureView);
            wgpuTextureRelease(surfaceTexture.texture);

            // Small delay to prevent 100% CPU usage
            SDL_Delay(16); // ~60 FPS
        }

        std::cout << "Render loop ended after " << frameCount << " frames" << std::endl;
    }

    void App::Terminate()
    {
        std::cout << "Terminating app..." << std::endl;

        if (m_surface)
        {
            wgpuSurfaceRelease(m_surface);
        }
        if (m_queue)
        {
            wgpuQueueRelease(m_queue);
        }
        if (m_device)
        {
            wgpuDeviceRelease(m_device);
        }
        if (m_adapter)
        {
            wgpuAdapterRelease(m_adapter);
        }
        if (m_instance)
        {
            wgpuInstanceRelease(m_instance);
        }
        if (m_window)
        {
            SDL_DestroyWindow(m_window);
        }

        SDL_Quit();

        m_running = false;
    }

} // namespace flint
