#include <iostream>

#include "Application.h"

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

WGPUStringView makeStringView(const char *str)
{
    return WGPUStringView{
        .data = str,
        .length = str ? strlen(str) : 0};
}

bool Application::Initialize()
{
    std::cout << "Starting SDL3 + WebGPU (Dawn 7187) application" << std::endl;

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Print info before creating window
    printVideoSystemInfo();

    // Set SDL hints for WebGPU compatibility (use string directly)
    SDL_SetHint("SDL_VIDEO_EXTERNAL_CONTEXT", "1"); // Don't create OpenGL context

    // Additional hints that might help
    SDL_SetHint("SDL_VIDEO_ALLOW_SCREENSAVER", "1"); // Allow screensaver

    // Create window WITHOUT any graphics API flags
    window = SDL_CreateWindow(
        "Flint & Timber",
        640, 480,
        SDL_WINDOW_RESIZABLE // Only basic window flags, no graphics API
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    printDetailedVideoInfo(window);

    std::cout << "SDL window created successfully (no graphics context)" << std::endl;

    std::cout << "Initializing WebGPU..." << std::endl;

    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;

    mInstance = wgpuCreateInstance(&instanceDesc);
    if (!mInstance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        return false;
    }

    std::cout << "✓ WebGPU instance created successfully" << std::endl;

    surface = SDL_GetWGPUSurface(mInstance, window);

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
    // adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
    // adapterOpts.backendType = WGPUBackendType_Undefined;
    // adapterOpts.forceFallbackAdapter = false;

    mAdapter = requestAdapterSync(mInstance, &adapterOpts);

    if (!mAdapter)
    {
        std::cerr << "Failed to request WebGPU adapter!" << std::endl;
        return false;
    }

    std::cout << "✓ Adapter obtained" << std::endl;

    // Request device
    std::cout << "\nRequesting WebGPU device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = makeStringView("Main Device");
    // deviceDesc.requiredFeatureCount = 0;
    // deviceDesc.requiredLimits = nullptr;
    // deviceDesc.defaultQueue.nextInChain = nullptr;
    // deviceDesc.defaultQueue.label = "The default queue";
    // deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const *message, void * /* pUserData */)
    // {
    //     std::cout << "Device lost: reason " << reason;
    //     if (message)
    //         std::cout << " (" << message << ")";
    //     std::cout << std::endl;
    // };
    device = requestDeviceSync(mInstance, mAdapter, &deviceDesc);

    if (!device)
    {
        std::cerr << "Failed to request WebGPU device!" << std::endl;
        return false;
    }

    std::cout << "✓ Device obtained" << std::endl;

    queue = wgpuDeviceGetQueue(device);

    if (!queue)
    {
        std::cerr << "Failed to request WebGPU queue!" << std::endl;
        return false;
    }

    std::cout << "✓ Queue obtained" << std::endl;

    // Configure surface
    WGPUSurfaceCapabilities surfaceCaps = {};
    wgpuSurfaceGetCapabilities(surface, mAdapter, &surfaceCaps);

    if (surfaceCaps.formatCount == 0)
    {
        std::cerr << "No surface formats available!" << std::endl;
        return false;
    }

    std::cout << "Available surface formats: " << surfaceCaps.formatCount << std::endl;

    // ======================

    // Configure the surface
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;

    // Configuration of the textures created for the underlying swap chain
    config.width = 640;
    config.height = 480;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = surfaceCaps.formats[0];

    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.device = device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);
    std::cout << "✓ Surface configured successfully" << std::endl;

    std::cout << "=== WebGPU initialization complete! ===" << std::endl;
    std::cout << "Press ESC or close window to exit." << std::endl;
    return true;
}

void Application::Terminate()
{
    // Release in reverse order of creation
    wgpuSurfaceUnconfigure(surface);
    if (queue) wgpuQueueRelease(queue);
    if (device) wgpuDeviceRelease(device);
    if (surface) wgpuSurfaceRelease(surface);
    if (mAdapter) wgpuAdapterRelease(mAdapter);
    if (mInstance) wgpuInstanceRelease(mInstance);

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Application::MainLoop()
{
    // Event loop
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            m_running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE)
            {
                m_running = false;
            }
            break;
        }
    }

    // Get the next target texture view
    WGPUTextureView targetView = GetNextSurfaceTextureView();
    if (!targetView)
    {
        // This can happen when resizing the window.
        return;
    }

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = makeStringView("Command Encoder");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
    if (!encoder)
    {
        std::cout << "ERROR: Failed to create command encoder!" << std::endl;
        wgpuTextureViewRelease(targetView);
        return;
    }

    // Create the render pass that clears the screen with our color
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;

    // The attachment part of the render pass descriptor describes the target texture of the pass
    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};

    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    if (!renderPass)
    {
        std::cout << "ERROR: Failed to create render pass!" << std::endl;
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(targetView);
        return;
    }
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    // Finally encode and submit the render pass
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = makeStringView("Command Buffer");
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);

    // Present the frame
    wgpuSurfacePresent(surface);

    // At the end of the frame, release the texture view
    wgpuTextureViewRelease(targetView);
}

bool Application::IsRunning()
{
    return m_running;
}

WGPUTextureView Application::GetNextSurfaceTextureView()
{
    // Get the surface texture
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
    {
        std::cout << "ERROR: Surface texture failed, status: " << surfaceTexture.status << std::endl;
        return nullptr;
    }

    // Create a view for this surface texture
    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = makeStringView("Surface texture view");
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    if (!targetView)
    {
        std::cout << "ERROR: Failed to create texture view!" << std::endl;
        return nullptr;
    }

    return targetView;
}