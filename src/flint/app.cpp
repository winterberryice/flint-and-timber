#include "flint/app.hpp"
#include "flint/player.hpp"
#include "flint/physics.hpp"
#include "flint/graphics/shaders.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace
{

    void printVideoSystemInfo()
    {
        // Get the current video driver
        const char *videoDriver = SDL_GetCurrentVideoDriver();

        printf("\n=== SDL3 Video System Information ===\n");
        printf("Current Video Driver: %s\n", videoDriver ? videoDriver : "NULL");

        if (videoDriver)
        {
            if (strcmp(videoDriver, "wayland") == 0)
            {
                printf("🌊 Running on WAYLAND\n");
            }
            else if (strcmp(videoDriver, "x11") == 0)
            {
                printf("🖥️  Running on X11\n");
            }
            else
            {
                printf("🤔 Running on: %s (other)\n", videoDriver);
            }
        }
        else
        {
            printf("❌ No video driver detected\n");
        }

        // List all available video drivers
        int numDrivers = SDL_GetNumVideoDrivers();
        printf("\nAvailable video drivers (%d total):\n", numDrivers);
        for (int i = 0; i < numDrivers; i++)
        {
            const char *driverName = SDL_GetVideoDriver(i);
            bool isCurrent = (videoDriver && strcmp(videoDriver, driverName) == 0);
            printf("  %s%s%s\n",
                   isCurrent ? "[ACTIVE] " : "         ",
                   driverName ? driverName : "NULL",
                   isCurrent ? " ←" : "");
        }
    }

    void printDetailedVideoInfo(SDL_Window *window)
    {
        const char *videoDriver = SDL_GetCurrentVideoDriver();

        printf("\n=== Detailed Video System Info ===\n");
        printf("SDL Video Driver: %s\n", videoDriver ? videoDriver : "NULL");

        if (!videoDriver)
            return;

        if (strcmp(videoDriver, "wayland") == 0)
        {
            printf("🌊 **WAYLAND SESSION DETECTED** 🌊\n");

            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);

                // Check for Wayland-specific properties
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER))
                {
                    void *waylandDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
                    void *waylandSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);

                    printf("  Wayland Display: %p\n", waylandDisplay);
                    printf("  Wayland Surface: %p\n", waylandSurface);
                    printf("  ✓ Native Wayland window detected\n");
                }
                else
                {
                    printf("  ❌ Wayland properties not available (running via XWayland?)\n");
                }
            }
        }
        else if (strcmp(videoDriver, "x11") == 0)
        {
            printf("🖥️  **X11 SESSION DETECTED** 🖥️\n");

            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);

                // Check for X11-specific properties
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER))
                {
                    void *x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
                    Uint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

                    printf("  X11 Display: %p\n", x11Display);
                    printf("  X11 Window: 0x%llx\n", (unsigned long long)x11Window);
                    printf("  ✓ Native X11 window detected\n");
                }
                else
                {
                    printf("  ❌ X11 properties not available\n");
                }
            }
        }
        else
        {
            printf("🤔 **OTHER DRIVER: %s** 🤔\n", videoDriver);
        }

        // Environment variable checks (additional context)
        const char *waylandDisplay = getenv("WAYLAND_DISPLAY");
        const char *x11Display = getenv("DISPLAY");
        const char *xdgSessionType = getenv("XDG_SESSION_TYPE");

        printf("\n=== Environment Variables ===\n");
        printf("WAYLAND_DISPLAY: %s\n", waylandDisplay ? waylandDisplay : "not set");
        printf("DISPLAY: %s\n", x11Display ? x11Display : "not set");
        printf("XDG_SESSION_TYPE: %s\n", xdgSessionType ? xdgSessionType : "not set");
    }

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
        : m_player(
              glm::vec3(8.0f, 20.0f, 8.0f), // Initial position
              -90.0f,                       // Initial yaw
              0.0f,                          // Initial pitch
              0.1f                           // Mouse sensitivity
          )
    {
    }

    // Add these variables for delta time calculation
    static Uint64 s_now = 0;
    static Uint64 s_last = 0;
    static float s_delta_time = 0.0f;

    // Add this for mouse grab state
    static bool s_mouse_grabbed = false;

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

        printVideoSystemInfo();

        // Create window
        m_window = SDL_CreateWindow("Flint & Timber", m_windowWidth, m_windowHeight, 0);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }

        printDetailedVideoInfo(m_window);

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

        // Get supported surface formats
        WGPUSurfaceCapabilities surfaceCapabilities;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &surfaceCapabilities);

        // Use the first supported format (this is the preferred one)
        m_surfaceFormat = surfaceCapabilities.formats[0];
        std::cout << "Using surface format: " << m_surfaceFormat << std::endl;

        // Configure the surface
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.device = m_device;
        surfaceConfig.format = m_surfaceFormat; // Common format
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        surfaceConfig.viewFormatCount = 0;
        surfaceConfig.viewFormats = nullptr;
        wgpuSurfaceCapabilitiesFreeMembers(surfaceCapabilities);
        //

        wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        std::cout << "WebGPU surface configured" << std::endl;

        std::cout << "Creating shaders..." << std::endl;

        const char *vertexShaderSource = R"(
struct Uniforms {
    viewProjectionMatrix: mat4x4<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.viewProjectionMatrix * vec4<f32>(input.position, 1.0);
    output.color = input.color;
    return output;
}
)";

        const char *fragmentShaderSource = R"(
@fragment
fn fs_main(@location(0) color: vec3<f32>) -> @location(0) vec4<f32> {
    return vec4<f32>(color, 1.0);
}
)";

        // Create shader module
        WGPUShaderModuleWGSLDescriptor shaderWGSLDesc = {};
        shaderWGSLDesc.chain.next = nullptr;
        shaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        shaderWGSLDesc.code = makeStringView(graphics::SHADER_SOURCE);

        WGPUShaderModuleDescriptor shaderDesc = {};
        shaderDesc.nextInChain = &shaderWGSLDesc.chain;
        shaderDesc.label = makeStringView("Main Shader");

        m_shaderModule = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);

        if (!m_shaderModule)
        {
            std::cerr << "Failed to create shader module!" << std::endl;
            return false;
        }
        std::cout << "Shader module created successfully" << std::endl;

        // Load the texture atlas
        if (!m_blockAtlas.loadFromFile(m_device, m_queue, "assets/textures/block/atlas.png"))
        {
            std::cerr << "Failed to load block atlas texture!" << std::endl;
            return false;
        }
        std::cout << "Block atlas texture loaded successfully" << std::endl;

        std::cout << "Setting up 3D components..." << std::endl;

        // Initialize chunk
        if (!m_chunk.initialize(m_device))
        {
            std::cerr << "Failed to initialize chunk!" << std::endl;
            return false;
        }

        // Create uniform buffer for camera matrices
        WGPUBufferDescriptor uniformBufferDesc = {};
        uniformBufferDesc.size = sizeof(glm::mat4); // Size of view-projection matrix
        uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        uniformBufferDesc.mappedAtCreation = false;

        m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);
        if (!m_uniformBuffer)
        {
            std::cerr << "Failed to create uniform buffer!" << std::endl;
            return false;
        }

        std::cout << "3D components ready!" << std::endl;

        std::cout << "Creating render pipeline..." << std::endl;

        // Create bind group layout for camera uniform
        WGPUBindGroupLayoutEntry cameraBindingLayout = {};
        cameraBindingLayout.binding = 0;
        cameraBindingLayout.visibility = WGPUShaderStage_Vertex;
        cameraBindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        cameraBindingLayout.buffer.minBindingSize = sizeof(glm::mat4);

        WGPUBindGroupLayoutDescriptor cameraBindGroupLayoutDesc = {};
        cameraBindGroupLayoutDesc.entryCount = 1;
        cameraBindGroupLayoutDesc.entries = &cameraBindingLayout;

        m_cameraBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &cameraBindGroupLayoutDesc);
        if (!m_cameraBindGroupLayout)
        {
            std::cerr << "Failed to create camera bind group layout!" << std::endl;
            return false;
        }

        // Create bind group layout for texture
        WGPUBindGroupLayoutEntry textureBindingLayout[2] = {};
        textureBindingLayout[0].binding = 0;
        textureBindingLayout[0].visibility = WGPUShaderStage_Fragment;
        textureBindingLayout[0].texture.sampleType = WGPUTextureSampleType_Float;
        textureBindingLayout[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        textureBindingLayout[1].binding = 1;
        textureBindingLayout[1].visibility = WGPUShaderStage_Fragment;
        textureBindingLayout[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor textureBindGroupLayoutDesc = {};
        textureBindGroupLayoutDesc.entryCount = 2;
        textureBindGroupLayoutDesc.entries = textureBindingLayout;

        m_textureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &textureBindGroupLayoutDesc);
        if (!m_textureBindGroupLayout)
        {
            std::cerr << "Failed to create texture bind group layout!" << std::endl;
            return false;
        }

        // Create pipeline layout
        WGPUBindGroupLayout bindGroupLayouts[] = {m_cameraBindGroupLayout, m_textureBindGroupLayout};
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 2;
        pipelineLayoutDesc.bindGroupLayouts = bindGroupLayouts;

        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);

        // Define vertex buffer layout for position + uv
        WGPUVertexAttribute vertexAttributes[2] = {};
        vertexAttributes[0].format = WGPUVertexFormat_Float32x3; // position
        vertexAttributes[0].offset = 0;
        vertexAttributes[0].shaderLocation = 0;

        vertexAttributes[1].format = WGPUVertexFormat_Float32x2; // uv
        vertexAttributes[1].offset = 3 * sizeof(float);
        vertexAttributes[1].shaderLocation = 1;

        WGPUVertexBufferLayout vertexBufferLayout = {};
        vertexBufferLayout.arrayStride = 5 * sizeof(float); // 3 for pos, 2 for uv
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout.attributeCount = 2;
        vertexBufferLayout.attributes = vertexAttributes;

        // Create render pipeline descriptor
        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Render Pipeline");
        pipelineDescriptor.layout = pipelineLayout;

        // Vertex state
        pipelineDescriptor.vertex.module = m_shaderModule;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        // Fragment state
        WGPUFragmentState fragmentState = {};
        fragmentState.module = m_shaderModule;
        fragmentState.entryPoint = makeStringView("fs_main");
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = m_surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive state
        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;

        // Multisample state
        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDescriptor);
        wgpuPipelineLayoutRelease(pipelineLayout);

        if (!m_renderPipeline)
        {
            std::cerr << "Failed to create render pipeline!" << std::endl;
            return false;
        }

        // Create bind group for camera
        WGPUBindGroupEntry cameraBinding = {};
        cameraBinding.binding = 0;
        cameraBinding.buffer = m_uniformBuffer;
        cameraBinding.offset = 0;
        cameraBinding.size = sizeof(glm::mat4);

        WGPUBindGroupDescriptor cameraBindGroupDesc = {};
        cameraBindGroupDesc.layout = m_cameraBindGroupLayout;
        cameraBindGroupDesc.entryCount = 1;
        cameraBindGroupDesc.entries = &cameraBinding;

        m_cameraBindGroup = wgpuDeviceCreateBindGroup(m_device, &cameraBindGroupDesc);

        // Create bind group for texture
        WGPUBindGroupEntry textureBindings[2] = {};
        textureBindings[0].binding = 0;
        textureBindings[0].textureView = m_blockAtlas.getView();
        textureBindings[1].binding = 1;
        textureBindings[1].sampler = m_blockAtlas.getSampler();

        WGPUBindGroupDescriptor textureBindGroupDesc = {};
        textureBindGroupDesc.layout = m_textureBindGroupLayout;
        textureBindGroupDesc.entryCount = 2;
        textureBindGroupDesc.entries = textureBindings;

        m_textureBindGroup = wgpuDeviceCreateBindGroup(m_device, &textureBindGroupDesc);

        std::cout << "Render pipeline created successfully" << std::endl;

        // ====
        m_running = true;
        return true;
    }

    void App::Run()
    {
        std::cout << "Running app..." << std::endl;

        s_last = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (m_running)
        {
            s_now = SDL_GetPerformanceCounter();
            s_delta_time = (s_now - s_last) / (float)SDL_GetPerformanceFrequency();
            s_last = s_now;

            // Handle events
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_EVENT_QUIT)
                {
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    s_mouse_grabbed = !s_mouse_grabbed;
                    SDL_SetWindowRelativeMouseMode(m_window, s_mouse_grabbed);
                }
                else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !s_mouse_grabbed)
                {
                    s_mouse_grabbed = true;
                    SDL_SetWindowRelativeMouseMode(m_window, s_mouse_grabbed);
                }

                // Player input handling
                if (s_mouse_grabbed)
                {
                    if (e.type == SDL_EVENT_MOUSE_MOTION)
                    {
                        m_player.process_mouse_movement(e.motion.xrel, e.motion.yrel);
                    }
                    m_player.handle_input(e);
                }
            }

            // Update player physics and state
            m_player.update(s_delta_time, m_chunk);

            // Render
            render();
        }
    }

    void App::render()
    {
        // Update camera matrix from player state and upload to GPU
        glm::vec3 eye = m_player.get_position() + glm::vec3(0.0f, physics::PLAYER_EYE_HEIGHT, 0.0f);
        float yaw_radians = glm::radians(m_player.get_yaw());
        float pitch_radians = glm::radians(m_player.get_pitch());

        glm::vec3 front;
        front.x = cos(yaw_radians) * cos(pitch_radians);
        front.y = sin(pitch_radians);
        front.z = sin(yaw_radians) * cos(pitch_radians);
        front = glm::normalize(front);

        glm::mat4 view = glm::lookAt(eye, eye + front, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)m_windowWidth / (float)m_windowHeight, 0.1f, 100.0f);

        glm::mat4 viewProjectionMatrix = proj * view;
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &viewProjectionMatrix, sizeof(glm::mat4));

        // Get surface texture
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
            colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f}; // Dark blue background
            colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.nextInChain = nullptr;
            renderPassDesc.label = {nullptr, 0};
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = nullptr;

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

            // Set the render pipeline
            wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);

            // Bind the camera and texture bind groups
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_cameraBindGroup, 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(renderPass, 1, m_textureBindGroup, 0, nullptr);

            // Draw the chunk
            m_chunk.render(renderPass);

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

    void App::Terminate()
    {
        std::cout << "Terminating app..." << std::endl;

        m_blockAtlas.cleanup();

        if (m_uniformBuffer)
            wgpuBufferRelease(m_uniformBuffer);
        if (m_cameraBindGroup)
            wgpuBindGroupRelease(m_cameraBindGroup);
        if (m_cameraBindGroupLayout)
            wgpuBindGroupLayoutRelease(m_cameraBindGroupLayout);
        if (m_textureBindGroup)
            wgpuBindGroupRelease(m_textureBindGroup);
        if (m_textureBindGroupLayout)
            wgpuBindGroupLayoutRelease(m_textureBindGroupLayout);
        if (m_renderPipeline)
        {
            wgpuRenderPipelineRelease(m_renderPipeline);
            m_renderPipeline = nullptr;
        }
        if (m_shaderModule)
        {
            wgpuShaderModuleRelease(m_shaderModule);
            m_shaderModule = nullptr;
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
