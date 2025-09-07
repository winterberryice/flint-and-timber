#include "flint/app.hpp"
#include <iostream>

namespace
{
    WGPUStringView makeStringView(const char *str)
    {
        return WGPUStringView{
            .data = str,
            .length = str ? strlen(str) : 0};
    }

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

        // Create window
        m_window = SDL_CreateWindow("WebGPU App", m_windowWidth, m_windowHeight, 0);
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

        // Request adapter
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.compatibleSurface = nullptr;
        adapterOptions.powerPreference = WGPUPowerPreference_Undefined;
        adapterOptions.backendType = WGPUBackendType_Undefined;
        adapterOptions.forceFallbackAdapter = false;

        AdapterRequestData adapterData;

        WGPURequestAdapterCallbackInfo callbackInfo = {};
        callbackInfo.nextInChain = nullptr;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents; // Changed this!
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
            // Try to process pending events
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

        // Create surface
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);
        if (!m_surface)
        {
            std::cerr << "Failed to create WebGPU surface" << std::endl;
            return false;
        }
        std::cout << "WebGPU surface created successfully" << std::endl;

        // Configure the surface
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.device = m_device;
        surfaceConfig.format = WGPUTextureFormat_BGRA8Unorm; // Common format
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        surfaceConfig.viewFormatCount = 0;
        surfaceConfig.viewFormats = nullptr;

        wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        std::cout << "WebGPU surface configured" << std::endl;

        std::cout << "Creating triangle vertex data..." << std::endl;

        // Triangle vertices in NDC coordinates (x, y, z)
        float triangleVertices[] = {
            0.0f, 0.5f, 0.0f,   // Top vertex
            -0.5f, -0.5f, 0.0f, // Bottom left
            0.5f, -0.5f, 0.0f   // Bottom right
        };

        // Create vertex buffer descriptor
        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.nextInChain = nullptr;
        vertexBufferDesc.label = makeStringView("Triangle Vertex Buffer");
        vertexBufferDesc.size = sizeof(triangleVertices);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.mappedAtCreation = false;

        // Create the vertex buffer
        m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
        if (!m_vertexBuffer)
        {
            std::cerr << "Failed to create vertex buffer!" << std::endl;
            return -1;
        }

        // Upload vertex data to GPU
        wgpuQueueWriteBuffer(m_queue, m_vertexBuffer, 0, triangleVertices, sizeof(triangleVertices));

        std::cout << "Vertex buffer created and data uploaded" << std::endl;

        std::cout << "Creating shaders..." << std::endl;

        // Vertex shader source code (WGSL)
        const char *vertexShaderSource = R"(
@vertex
fn vs_main(@location(0) position: vec3f) -> @builtin(position) vec4f {
    return vec4f(position, 1.0);
}
)";

        // Fragment shader source code (WGSL)
        const char *fragmentShaderSource = R"(
@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(1.0, 0.0, 0.0, 1.0); // Red color
}
)";

        // Create vertex shader module
        WGPUShaderModuleWGSLDescriptor vertexShaderWGSLDesc = {};
        vertexShaderWGSLDesc.chain.next = nullptr;
        vertexShaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexShaderWGSLDesc.code = makeStringView(vertexShaderSource);

        WGPUShaderModuleDescriptor vertexShaderDesc = {};
        vertexShaderDesc.nextInChain = &vertexShaderWGSLDesc.chain;
        vertexShaderDesc.label = makeStringView("Vertex Shader");

        m_vertexShader = wgpuDeviceCreateShaderModule(m_device, &vertexShaderDesc);

        // Create fragment shader module
        WGPUShaderModuleWGSLDescriptor fragmentShaderWGSLDesc = {};
        fragmentShaderWGSLDesc.chain.next = nullptr;
        fragmentShaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentShaderWGSLDesc.code = makeStringView(fragmentShaderSource);

        WGPUShaderModuleDescriptor fragmentShaderDesc = {};
        fragmentShaderDesc.nextInChain = &fragmentShaderWGSLDesc.chain;
        fragmentShaderDesc.label = makeStringView("Fragment Shader");

        m_fragmentShader = wgpuDeviceCreateShaderModule(m_device, &fragmentShaderDesc);

        if (!m_vertexShader || !m_fragmentShader)
        {
            std::cerr << "Failed to create shaders!" << std::endl;
            return false; // Assuming your init function returns bool
        }

        std::cout << "Shaders created successfully" << std::endl;

        // ====
        m_running = true;
        return true;
    }

    void App::Run()
    {
        std::cout << "Running app..." << std::endl;

        SDL_Event e;
        while (m_running)
        {
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

            // Simple render - clear to a color
            WGPUSurfaceTexture surfaceTexture;
            wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

            if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
            {
                WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

                WGPUCommandEncoderDescriptor encoderDesc = {};
                encoderDesc.nextInChain = nullptr;
                encoderDesc.label = {nullptr, 0};

                WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

                WGPURenderPassColorAttachment colorAttachment = {};
                colorAttachment.view = textureView;
                colorAttachment.resolveTarget = nullptr;
                colorAttachment.loadOp = WGPULoadOp_Clear;
                colorAttachment.storeOp = WGPUStoreOp_Store;
                colorAttachment.clearValue = {0.2f, 0.3f, 0.8f, 1.0f}; // Nice blue color
                colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

                WGPURenderPassDescriptor renderPassDesc = {};
                renderPassDesc.nextInChain = nullptr;
                renderPassDesc.label = {nullptr, 0};
                renderPassDesc.colorAttachmentCount = 1;
                renderPassDesc.colorAttachments = &colorAttachment;
                renderPassDesc.depthStencilAttachment = nullptr;

                WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
                wgpuRenderPassEncoderEnd(renderPass);

                WGPUCommandBufferDescriptor cmdBufferDesc = {};
                cmdBufferDesc.nextInChain = nullptr;
                cmdBufferDesc.label = {nullptr, 0};

                WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
                wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

                wgpuSurfacePresent(m_surface);

                // Cleanup
                wgpuCommandBufferRelease(cmdBuffer);
                wgpuRenderPassEncoderRelease(renderPass);
                wgpuCommandEncoderRelease(encoder);
                wgpuTextureViewRelease(textureView);
            }

            wgpuTextureRelease(surfaceTexture.texture);
        }
    }

    void App::Terminate()
    {
        std::cout << "Terminating app..." << std::endl;

        if (m_vertexShader)
        {
            wgpuShaderModuleRelease(m_vertexShader);
            m_vertexShader = nullptr;
        }
        if (m_fragmentShader)
        {
            wgpuShaderModuleRelease(m_fragmentShader);
            m_fragmentShader = nullptr;
        }
        if (m_vertexBuffer)
        {
            wgpuBufferRelease(m_vertexBuffer);
        }
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
