#include "app.hpp"
#include <iostream>
#include "wgpu_core.h"

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

}

namespace flint
{
    App::App()
    {
        std::cout << "Initializing app..." << std::endl;

        m_windowWidth = 800;
        m_windowHeight = 600;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            throw std::runtime_error("Failed to initialize SDL");
        }

        printVideoSystemInfo();

        // Create window
        m_window = SDL_CreateWindow("Flint & Timber", m_windowWidth, m_windowHeight, 0);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            throw std::runtime_error("Failed to create window");
        }

        printDetailedVideoInfo(m_window);

        std::cout << "SDL initialized successfully" << std::endl;

        init_wgpu(
            m_windowWidth,
            m_windowHeight,
            m_window,
            m_instance,
            m_surface,
            m_surfaceFormat,
            m_adapter,
            m_device,
            m_queue //
        );

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
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        // Upload vertex data to GPU
        wgpuQueueWriteBuffer(m_queue, m_vertexBuffer, 0, triangleVertices, sizeof(triangleVertices));

        std::cout << "Vertex buffer created and data uploaded" << std::endl;

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
            throw std::runtime_error("Failed to create shaders!");
        }

        std::cout << "Shaders created successfully" << std::endl;

        std::cout << "Setting up 3D components..." << std::endl;

        // Initialize camera
        m_camera.setPosition(glm::vec3(2.0f, 2.0f, 3.0f));             // Position camera to see cube at angle
        m_camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));               // Look at origin
        m_camera.setPerspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f); // FOV, aspect ratio, near, far

        // Initialize cube mesh
        if (!m_cubeMesh.initialize(m_device))
        {
            std::cerr << "Failed to initialize cube mesh!" << std::endl;
            throw std::runtime_error("Failed to initialize cube mesh!");
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
            throw std::runtime_error("Failed to create uniform buffer!");
        }

        std::cout << "3D components ready!" << std::endl;

        std::cout << "Creating render pipeline..." << std::endl;

        // Create bind group layout for uniforms first
        WGPUBindGroupLayoutEntry bindingLayout = {};
        bindingLayout.binding = 0;
        bindingLayout.visibility = WGPUShaderStage_Vertex;
        bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayout.buffer.minBindingSize = sizeof(glm::mat4);

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &bindingLayout;

        m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDesc);
        if (!m_bindGroupLayout)
        {
            std::cerr << "Failed to create bind group layout!" << std::endl;
            throw std::runtime_error("Failed to create bind group layout!");
        }

        // Create pipeline layout using our bind group layout
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;

        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);

        // NEW: Define vertex buffer layout for position + color
        WGPUVertexAttribute vertexAttributes[2] = {};

        // Position attribute (location 0)
        vertexAttributes[0].format = WGPUVertexFormat_Float32x3; // vec3f position
        vertexAttributes[0].offset = 0;
        vertexAttributes[0].shaderLocation = 0;

        // Color attribute (location 1)
        vertexAttributes[1].format = WGPUVertexFormat_Float32x3; // vec3f color
        vertexAttributes[1].offset = 3 * sizeof(float);          // After position
        vertexAttributes[1].shaderLocation = 1;

        WGPUVertexBufferLayout vertexBufferLayout = {};
        vertexBufferLayout.arrayStride = 6 * sizeof(float); // 3 floats position + 3 floats color
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout.attributeCount = 2; // position + color
        vertexBufferLayout.attributes = vertexAttributes;

        // Create render pipeline descriptor
        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Cube Render Pipeline");

        // Vertex state
        pipelineDescriptor.vertex.module = m_vertexShader;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        // Fragment state
        WGPUFragmentState fragmentState = {};
        fragmentState.module = m_fragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");

        // Color target (what we're rendering to)
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = m_surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive state - IMPORTANT: Enable depth testing for 3D
        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back; // Enable back-face culling

        // Multisample state
        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        // Use our custom layout (not auto)
        pipelineDescriptor.layout = pipelineLayout;

        // Create the pipeline
        m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDescriptor);

        // Clean up temporary layout
        wgpuPipelineLayoutRelease(pipelineLayout);

        if (!m_renderPipeline)
        {
            std::cerr << "Failed to create render pipeline!" << std::endl;
            throw std::runtime_error("Failed to create render pipeline!");
        }

        // Create bind group for uniforms
        WGPUBindGroupEntry binding = {};
        binding.binding = 0;
        binding.buffer = m_uniformBuffer;
        binding.offset = 0;
        binding.size = sizeof(glm::mat4);

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = m_bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;

        m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &bindGroupDesc);

        std::cout << "Render pipeline created successfully" << std::endl;

        // ====
        m_running = true;
    }

    void App::run()
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

            // Render
            render();
        }
    }

    void App::render()
    {
        // Update camera matrix and upload to GPU
        glm::mat4 viewProjectionMatrix = m_camera.getViewProjectionMatrix();
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

            // NEW: 3D Cube rendering instead of triangle
            wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);

            // Bind the uniform buffer (camera matrix)
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);

            // Draw the cube using our mesh class
            m_cubeMesh.render(renderPass);

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

    App::~App()
    {
        std::cout << "Terminating app..." << std::endl;

        if (m_uniformBuffer)
            wgpuBufferRelease(m_uniformBuffer);
        if (m_bindGroup)
            wgpuBindGroupRelease(m_bindGroup);
        if (m_bindGroupLayout)
            wgpuBindGroupLayoutRelease(m_bindGroupLayout);
        if (m_renderPipeline)
        {
            wgpuRenderPipelineRelease(m_renderPipeline);
            m_renderPipeline = nullptr;
        }
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
