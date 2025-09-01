#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <sdl3webgpu.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>

// Global WebGPU objects
WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUDevice device = nullptr;
WGPUQueue queue = nullptr;
WGPUSurface surface = nullptr;

// Additions for triangle rendering
WGPURenderPipeline pipeline = nullptr;
WGPUBuffer vertexBuffer = nullptr;
std::vector<float> vertexData;

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
    //
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

const char *shaderSource = R"(
    @vertex
    fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
        return vec4f(in_vertex_position, 0.0, 1.0);
    }

    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4f(1.0, 0.0, 0.0, 1.0); // Red
    }
)";

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

    // Store the surface format
    WGPUTextureFormat surfaceFormat = surfaceCaps.formats[0];

    wgpuSurfaceConfigure(surface, &surfaceConfig);
    std::cout << "✓ Surface configured successfully" << std::endl;

    // Create vertex buffer
    vertexData = {
        // x0, y0
        0.0f, 0.5f,
        // x1, y1
        -0.5f, -0.5f,
        // x2, y2
        0.5f, -0.5f};

    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = vertexData.size() * sizeof(float);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDesc.mappedAtCreation = false;
    vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertexData.data(), bufferDesc.size);
    std::cout << "✓ Vertex buffer created and populated" << std::endl;

    // Create shader module
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    shaderCodeDesc.chain.sType = (WGPUSType)0x00000011; // WGPUSType_ShaderModuleWGSLDescriptor
    shaderCodeDesc.code = makeStringView(shaderSource);
    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = (WGPUChainedStruct*)&shaderCodeDesc;
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    std::cout << "✓ Shader module created" << std::endl;

    // Create render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.nextInChain = nullptr;

    // Vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = makeStringView("vs_main");
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    // Vertex fetch
    WGPUVertexAttribute attribute = {};
    attribute.shaderLocation = 0;
    attribute.offset = 0;
    attribute.format = WGPUVertexFormat_Float32x2;

    WGPUVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 2 * sizeof(float);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &attribute;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;

    // Primitive assembly and rasterization
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;

    // Fragment shader
    WGPUFragmentState fragmentState = {};
    fragmentState.nextInChain = nullptr;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = makeStringView("fs_main");
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    pipelineDesc.fragment = &fragmentState;

    // Configure blend state
    WGPUBlendState blendState = {};
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Depth and stencil buffers
    pipelineDesc.depthStencil = nullptr;

    // Multi-sampling
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Pipeline layout
    pipelineDesc.layout = nullptr;

    pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    std::cout << "✓ Render pipeline created" << std::endl;

    // We no longer need the shader module
    wgpuShaderModuleRelease(shaderModule);

    std::cout << "=== WebGPU initialization complete! ===" << std::endl;
    return true;
}

void render()
{
    static int renderCount = 0;
    renderCount++;

    if (!surface || !device || !queue)
    {
        std::cout << "ERROR: Missing WebGPU resources!" << std::endl;
        return;
    }

    // Get next surface texture
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

    // Check for any success status (1 = SuccessOptimal, 2 = SuccessSuboptimal)
    if (!surfaceTexture.texture || (surfaceTexture.status != 1 && surfaceTexture.status != 2))
    {
        std::cout << "ERROR: Surface texture failed, status: " << surfaceTexture.status << std::endl;
        return;
    }

    if (renderCount <= 5)
    {
        std::cout << "Frame " << renderCount << ": Got surface texture successfully (status: " << surfaceTexture.status << ")" << std::endl;
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
        std::cout << "ERROR: Failed to create texture view!" << std::endl;
        return;
    }

    if (renderCount <= 5)
    {
        std::cout << "Frame " << renderCount << ": Created texture view" << std::endl;
    }

    // Create command encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = makeStringView("Command Encoder");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
    if (!encoder)
    {
        std::cout << "ERROR: Failed to create command encoder!" << std::endl;
        wgpuTextureViewRelease(textureView);
        return;
    }

    if (renderCount <= 5)
    {
        std::cout << "Frame " << renderCount << ": Created command encoder" << std::endl;
    }

    // Create render pass with BLACK clear color
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.nextInChain = nullptr;
    colorAttachment.view = textureView;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;   // Clear the screen
    colorAttachment.storeOp = WGPUStoreOp_Store; // Store the result
    colorAttachment.clearValue = {0.0f, 0.0f, 1.0f, 1.0f}; // Blue

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.label = makeStringView("Render Pass");
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    if (!renderPass)
    {
        std::cout << "ERROR: Failed to create render pass!" << std::endl;
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(textureView);
        return;
    }

    // Draw the triangle
    wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, vertexData.size() * sizeof(float));
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    // Finish command buffer
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = makeStringView("Command Buffer");

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);

    // Submit commands
    wgpuQueueSubmit(queue, 1, &commandBuffer);

    if (renderCount <= 5)
    {
        std::cout << "Frame " << renderCount << ": Commands submitted - executing GPU clear now!" << std::endl;
    }

    // Present the frame
    wgpuSurfacePresent(surface);

    if (renderCount <= 5)
    {
        std::cout << "Frame " << renderCount << ": *** FRAME PRESENTED - WINDOW SHOULD BE BRIGHT PURPLE NOW! ***" << std::endl;
    }

    // Clean up
    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(textureView);
}

void cleanup()
{
    std::cout << "Cleaning up WebGPU resources..." << std::endl;

    try
    {
        if (pipeline)
        {
            wgpuRenderPipelineRelease(pipeline);
            pipeline = nullptr;
            std::cout << "✓ Pipeline released" << std::endl;
        }
        if (vertexBuffer)
        {
            wgpuBufferDestroy(vertexBuffer);
            wgpuBufferRelease(vertexBuffer);
            vertexBuffer = nullptr;
            std::cout << "✓ Vertex buffer released" << std::endl;
        }
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

// More detailed function with window-specific info
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

int main(int argc, char *argv[])
{
    std::cout << "Starting SDL3 + WebGPU (Dawn 7187) application" << std::endl;

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Print info before creating window
    printVideoSystemInfo();

    // Set SDL hints for WebGPU compatibility (use string directly)
    SDL_SetHint("SDL_VIDEO_EXTERNAL_CONTEXT", "1"); // Don't create OpenGL context

    // Additional hints that might help
    SDL_SetHint("SDL_VIDEO_ALLOW_SCREENSAVER", "1"); // Allow screensaver

    // Create window WITHOUT any graphics API flags
    SDL_Window *window = SDL_CreateWindow(
        "Flint & Timber",
        800, 600,
        SDL_WINDOW_RESIZABLE // Only basic window flags, no graphics API
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Print detailed info with window
    printDetailedVideoInfo(window);

    std::cout << "SDL window created successfully (no graphics context)" << std::endl;

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
