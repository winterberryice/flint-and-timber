#include <GLFW/glfw3.h>
#if defined(WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#if defined(__APPLE__)
#include <QuartzCore/CAMetalLayer.h>
#endif
#include <webgpu/webgpu.h>
#include <iostream>
#include <cstring>

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUDevice device = nullptr;
WGPUQueue queue = nullptr;
WGPUSurface surface = nullptr;
WGPUShaderModule shaderModule = nullptr;
WGPURenderPipeline pipeline = nullptr;

// Global variables for callbacks
bool adapterReady = false;
bool deviceReady = false;

// Helper function to create WGPUStringView from C string
WGPUStringView makeStringView(const char *str)
{
    WGPUStringView view = {};
    if (str)
    {
        view.data = str;
        view.length = strlen(str);
    }
    return view;
}

// Simple callback for adapter request
void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapterResult, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        adapter = adapterResult;
        std::cout << "Adapter obtained successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to get adapter" << std::endl;
    }
    adapterReady = true;
}

// Simple callback for device request
void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice deviceResult, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        device = deviceResult;
        std::cout << "Device obtained successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to get device" << std::endl;
    }
    deviceReady = true;
}

bool InitWebGPU(WGPUSurface surface)
{
    // Request adapter
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapterOpts.compatibleSurface = surface;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = nullptr;
    callbackInfo.userdata2 = nullptr;

    WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapterOpts, callbackInfo);

    // Simple polling for adapter
    while (!adapterReady)
    {
        wgpuInstanceProcessEvents(instance);
    }

    if (!adapter)
    {
        std::cerr << "Failed to obtain adapter!" << std::endl;
        return false;
    }

    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = makeStringView("Primary Device");

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallbackInfo.callback = onDeviceRequestEnded;
    deviceCallbackInfo.userdata1 = nullptr;
    deviceCallbackInfo.userdata2 = nullptr;

    WGPUFuture deviceFuture = wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);

    // Simple polling for device
    while (!deviceReady)
    {
        wgpuInstanceProcessEvents(instance);
    }

    if (!device)
    {
        std::cerr << "Failed to obtain device!" << std::endl;
        return false;
    }

    // Get the queue
    queue = wgpuDeviceGetQueue(device);
    if (!queue)
    {
        std::cerr << "Failed to get device queue!" << std::endl;
        return false;
    }

    std::cout << "WebGPU setup complete!" << std::endl;
    return true;
}

void Cleanup()
{
    if (pipeline)
        wgpuRenderPipelineRelease(pipeline);
    if (shaderModule)
        wgpuShaderModuleRelease(shaderModule);
    if (queue)
        wgpuQueueRelease(queue);
    if (device)
        wgpuDeviceRelease(device);
    if (surface)
        wgpuSurfaceRelease(surface);
    if (adapter)
        wgpuAdapterRelease(adapter);
    if (instance)
        wgpuInstanceRelease(instance);
}

void Start()
{
    if (!glfwInit())
    {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(kWidth, kHeight, "WebGPU window", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return;
    }

    // Create instance
    WGPUInstanceDescriptor instanceDesc = {};
    instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }
    std::cout << "WebGPU instance created successfully!" << std::endl;

    // Create a surface
    {
#if defined(WIN32)
        WGPUSurfaceDescriptorFromWindowsHWND hwnd_desc = {};
        hwnd_desc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
        hwnd_desc.hinstance = GetModuleHandle(NULL);
        hwnd_desc.hwnd = glfwGetWin32Window(window);
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&hwnd_desc);
        surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
#elif defined(__linux__)
        WGPUSurfaceDescriptorFromXlibWindow xlib_desc = {};
        xlib_desc.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        xlib_desc.display = glfwGetX11Display();
        xlib_desc.window = glfwGetX11Window(window);
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&xlib_desc);
        surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
#elif defined(__APPLE__)
        NSWindow *nsWindow = glfwGetCocoaWindow(window);
        [nsWindow.contentView setWantsLayer:YES];
        CAMetalLayer *metalLayer = [CAMetalLayer layer];
        [nsWindow.contentView setLayer:metalLayer];
        WGPUSurfaceDescriptorFromMetalLayer metal_desc = {};
        metal_desc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
        metal_desc.layer = metalLayer;
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&metal_desc);
        surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
#endif
    }

    if (!surface)
    {
        std::cerr << "Failed to create WebGPU surface!" << std::endl;
        Cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }
    std::cout << "WebGPU surface created successfully!" << std::endl;

    // Initialize WebGPU
    if (!InitWebGPU(surface))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    // Configure the surface
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
    WGPUTextureFormat swapChainFormat = capabilities.formats[0];

    WGPUSurfaceConfiguration config = {};
    config.device = device;
    config.format = swapChainFormat;
    config.width = kWidth;
    config.height = kHeight;
    config.presentMode = WGPUPresentMode_Fifo;
    wgpuSurfaceConfigure(surface, &config);

    // Create a shader module
    const char *shaderSource = R"(
        @vertex
        fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
            var pos = array<vec2<f32>, 3>(
                vec2<f32>(0.0, 0.5),
                vec2<f32>(-0.5, -0.5),
                vec2<f32>(0.5, -0.5)
            );

            return vec4<f32>(pos[in_vertex_index], 0.0, 1.0);
        }

        @fragment
        fn fs_main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 0.0, 0.0, 1.0); // Red
        }
    )";

    WGPUShaderModuleWGSLDescriptor wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code = makeStringView(shaderSource);

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgslDesc);

    shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    if (!shaderModule)
    {
        std::cerr << "Failed to create shader module!" << std::endl;
        Cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }
    std::cout << "Shader module created successfully!" << std::endl;

    // Create a render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};

    // Vertex state
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = makeStringView("vs_main");
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;

    // Fragment state
    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format = swapChainFormat;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = makeStringView("fs_main");
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTargetState;
    pipelineDesc.fragment = &fragmentState;

    // Primitive state
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;

    // Multisample state
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = 0xFFFFFFFF;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Create pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = 0;
    layoutDesc.bindGroupLayouts = nullptr;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
    pipelineDesc.layout = layout;

    pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    if (!pipeline)
    {
        std::cerr << "Failed to create render pipeline!" << std::endl;
        Cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }
    std::cout << "Render pipeline created successfully!" << std::endl;

    // Release the layout, it is no longer needed after pipeline creation
    wgpuPipelineLayoutRelease(layout);

    std::cout << "Starting render loop..." << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        {
            if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Timeout &&
                surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Outdated)
            {
                std::cerr << "Failed to get next swap chain texture, status: " << surfaceTexture.status << std::endl;
            }
            if (surfaceTexture.texture)
                wgpuTextureRelease(surfaceTexture.texture);
            continue;
        }

        WGPUTextureView nextTexture = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
        if (!nextTexture)
        {
            std::cerr << "Failed to create texture view from swap chain texture!" << std::endl;
            if (surfaceTexture.texture)
                wgpuTextureRelease(surfaceTexture.texture);
            continue;
        }

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = makeStringView("Frame Command Encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = nextTexture;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.0, 0.0, 0.0, 1.0}; // Black

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(renderPass);

        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = makeStringView("Frame Command Buffer");
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);

        wgpuQueueSubmit(queue, 1, &commandBuffer);

        wgpuSurfacePresent(surface);

        // Clean up frame resources
        wgpuTextureViewRelease(nextTexture);
        wgpuTextureRelease(surfaceTexture.texture);
        wgpuRenderPassEncoderRelease(renderPass);
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(encoder);

        // Process WebGPU events
        wgpuInstanceProcessEvents(instance);
    }

    Cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Application finished." << std::endl;
}

int main()
{
    Start();
    return 0;
}
