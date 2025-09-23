#include "main.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include "debug.h"
#include "shader.wgsl.h" // For shader sources

// Platform-specific function to create a WGPUSurface from an SDL_Window
WGPUSurface createSurface(WGPUInstance instance, SDL_Window *window)
{
    return SDL_GetWGPUSurface(instance, window);
}

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
        AdapterRequestData *data = static_cast<AdapterRequestData *>(userdata1);
        if (status == WGPURequestAdapterStatus_Success)
        {
            data->adapter = adapter;
        }
        data->done = true;
    }

    // Helper for synchronous device request
    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    static void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
    {
        DeviceRequestData *data = static_cast<DeviceRequestData *>(userdata1);
        if (status == WGPURequestDeviceStatus_Success)
        {
            data->device = device;
        }
        data->done = true;
    }
}

namespace flint
{

    Application::Application()
    {
        init();
    }

    Application::~Application()
    {
        cleanup();
    }

    void Application::run()
    {
        mainLoop();
    }

    void Application::init()
    {
        // --- SDL Init ---
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error("Could not initialize SDL");

        mSize = {1280, 720};
        mWindow = SDL_CreateWindow("Flint (C++)", mSize.x, mSize.y, SDL_WINDOW_RESIZABLE);
        if (!mWindow)
            throw std::runtime_error("Could not create SDL window");

        // --- WebGPU Init ---
        mInstance = wgpuCreateInstance(nullptr);
        if (!mInstance)
            throw std::runtime_error("Could not create WGPU instance");

        mSurface = createSurface(mInstance, mWindow);
        if (!mSurface)
            throw std::runtime_error("Could not create WGPU surface");

        getWgpuDevice(); // Gets adapter and device

        mQueue = wgpuDeviceGetQueue(mDevice);

        initSwapChain();
        initPipeline();

        setMouseGrab(true);
    }

    void Application::getWgpuDevice()
    {
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.compatibleSurface = mSurface;
        AdapterRequestData adapterData = {};
        wgpuInstanceRequestAdapter(mInstance, &adapterOptions, OnAdapterReceived, &adapterData);
        while (!adapterData.done) { wgpuInstanceProcessEvents(mInstance); }
        mAdapter = adapterData.adapter;
        if (!mAdapter) throw std::runtime_error("Failed to get WGPU adapter.");

        DeviceRequestData deviceData = {};
        wgpuAdapterRequestDevice(mAdapter, nullptr, OnDeviceReceived, &deviceData);
        while (!deviceData.done) { wgpuInstanceProcessEvents(mInstance); }
        mDevice = deviceData.device;
        if (!mDevice) throw std::runtime_error("Failed to get WGPU device.");
    }

    void Application::initSwapChain()
    {
        WGPUSurfaceCapabilities caps;
        wgpuSurfaceGetCapabilities(mSurface, mAdapter, &caps);
        mSurfaceConfig.format = caps.formats[0];
        wgpuSurfaceCapabilitiesFreeMembers(caps);

        mSurfaceConfig.device = mDevice;
        mSurfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        mSurfaceConfig.width = mSize.x;
        mSurfaceConfig.height = mSize.y;
        mSurfaceConfig.presentMode = WGPUPresentMode_Fifo;
        mSurfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        wgpuSurfaceConfigure(mSurface, &mSurfaceConfig);
    }

    void Application::initCameraAndChunk() {
        // This function is now empty as its logic is moved into init() and initPipeline()
        // to match the user's example. The hardcoded mesh is now created in initPipeline().
    }

    void Application::initPipeline()
    {
        // --- Hardcoded Cube Data ---
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        glm::vec3 center = {8.0f, 64.0f, 8.0f};
        glm::vec3 corners[8] = {
            center + glm::vec3(-0.5f, -0.5f, -0.5f), center + glm::vec3(0.5f, -0.5f, -0.5f),
            center + glm::vec3(0.5f,  0.5f, -0.5f), center + glm::vec3(-0.5f,  0.5f, -0.5f),
            center + glm::vec3(-0.5f, -0.5f,  0.5f), center + glm::vec3(0.5f, -0.5f,  0.5f),
            center + glm::vec3(0.5f,  0.5f,  0.5f), center + glm::vec3(-0.5f,  0.5f,  0.5f)
        };
        struct Face { uint16_t i[4]; glm::vec3 c; };
        Face faces[6] = {
            {{0, 3, 2, 1}, {1, 0, 0}}, {{5, 6, 7, 4}, {0, 1, 0}},
            {{1, 2, 6, 5}, {0, 0, 1}}, {{4, 7, 3, 0}, {1, 1, 0}},
            {{3, 7, 6, 2}, {1, 0, 1}}, {{4, 0, 1, 5}, {0, 1, 1}}
        };
        uint16_t vertex_offset = 0;
        for (const auto &face : faces) {
            for(int i = 0; i < 4; ++i) vertices.push_back({corners[face.i[i]], face.c});
            indices.insert(indices.end(), {
                (uint16_t)(vertex_offset+0), (uint16_t)(vertex_offset+1), (uint16_t)(vertex_offset+2),
                (uint16_t)(vertex_offset+0), (uint16_t)(vertex_offset+2), (uint16_t)(vertex_offset+3)
            });
            vertex_offset += 4;
        }
        mNumIndices = indices.size();

        // --- Create Buffers ---
        WGPUBufferDescriptor vb_desc = {};
        vb_desc.size = vertices.size() * sizeof(Vertex);
        vb_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        mVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &vb_desc);
        wgpuQueueWriteBuffer(mQueue, mVertexBuffer, 0, vertices.data(), vb_desc.size);

        WGPUBufferDescriptor ib_desc = {};
        ib_desc.size = indices.size() * sizeof(uint16_t);
        ib_desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        mIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &ib_desc);
        wgpuQueueWriteBuffer(mQueue, mIndexBuffer, 0, indices.data(), ib_desc.size);

        WGPUBufferDescriptor ub_desc = {};
        ub_desc.size = sizeof(CameraUniform);
        ub_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        mUniformBuffer = wgpuDeviceCreateBuffer(mDevice, &ub_desc);

        // --- Shaders ---
        mVertexShader = createShaderModule(VERTEX_SHADER_SOURCE);
        mFragmentShader = createShaderModule(FRAGMENT_SHADER_SOURCE);

        // --- Bind Group Layout ---
        WGPUBindGroupLayoutEntry bgl_entry = {};
        bgl_entry.binding = 0;
        bgl_entry.visibility = WGPUShaderStage_Vertex;
        bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;
        bgl_entry.buffer.minBindingSize = sizeof(CameraUniform);
        WGPUBindGroupLayoutDescriptor bgl_desc = {};
        bgl_desc.entryCount = 1;
        bgl_desc.entries = &bgl_entry;
        mBindGroupLayout = wgpuDeviceCreateBindGroupLayout(mDevice, &bgl_desc);

        // --- Pipeline Layout ---
        WGPUPipelineLayoutDescriptor pl_desc = {};
        pl_desc.bindGroupLayoutCount = 1;
        pl_desc.bindGroupLayouts = &mBindGroupLayout;
        mPipelineLayout = wgpuDeviceCreatePipelineLayout(mDevice, &pl_desc);

        // --- Render Pipeline ---
        WGPURenderPipelineDescriptor rp_desc = {};
        rp_desc.layout = mPipelineLayout;
        rp_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        rp_desc.primitive.frontFace = WGPUFrontFace_CCW;
        rp_desc.primitive.cullMode = WGPUCullMode_Back;
        rp_desc.multisample.count = 1;
        rp_desc.depthStencil = nullptr;

        WGPUVertexState vs = {};
        vs.module = mVertexShader;
        vs.entryPoint = "vs_main";
        WGPUVertexBufferLayout vbl = Vertex::getLayout();
        vs.bufferCount = 1;
        vs.buffers = &vbl;
        rp_desc.vertex = vs;

        WGPUFragmentState fs = {};
        fs.module = mFragmentShader;
        fs.entryPoint = "fs_main";
        WGPUColorTargetState cts = {};
        cts.format = mSurfaceConfig.format;
        cts.writeMask = WGPUColorWriteMask_All;
        fs.targetCount = 1;
        fs.targets = &cts;
        rp_desc.fragment = &fs;

        mRenderPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &rp_desc);

        // --- Bind Group ---
        WGPUBindGroupEntry bg_entry = {};
        bg_entry.binding = 0;
        bg_entry.buffer = mUniformBuffer;
        bg_entry.offset = 0;
        bg_entry.size = sizeof(CameraUniform);
        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout = mBindGroupLayout;
        bg_desc.entryCount = 1;
        bg_desc.entries = &bg_entry;
        mBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bg_desc);
    }

    void Application::mainLoop()
    {
        while (mIsRunning)
        {
            handleEvents();
            update();
            render();
        }
    }

    void Application::handleEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) mIsRunning = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) mIsRunning = false;
        }
    }

    void Application::update()
    {
        // Simplified camera logic for a stationary view
        mCamera = Camera(
            {8.0f, 65.0f, 12.0f}, // eye
            {8.0f, 64.0f, 8.0f},  // target
            {0.0f, 1.0f, 0.0f},   // up
            (float)mSize.x / (float)mSize.y,
            45.0f, 0.1f, 100.0f
        );
        mCameraUniform.updateViewProj(mCamera);
        wgpuQueueWriteBuffer(mQueue, mUniformBuffer, 0, &mCameraUniform, sizeof(CameraUniform));
    }

    void Application::render()
    {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(mSurface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) return;

        WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, nullptr);

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = view;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f};

        WGPURenderPassDescriptor passDesc = {};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;
        passDesc.depthStencilAttachment = nullptr;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);
        wgpuRenderPassEncoderSetPipeline(pass, mRenderPipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, mBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, mVertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(pass, mIndexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(pass, mNumIndices, 1, 0, 0, 0);
        wgpuRenderPassEncoderEnd(pass);

        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(mQueue, 1, &cmd);
        wgpuSurfacePresent(mSurface);

        wgpuTextureViewRelease(view);
        wgpuTextureRelease(surfaceTexture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandBufferRelease(cmd);
    }

    void Application::resize(int new_width, int new_height)
    {
        if (new_width > 0 && new_height > 0)
        {
            mSize = {(unsigned int)new_width, (unsigned int)new_height};
            initSwapChain();
        }
    }

    void Application::setMouseGrab(bool grab)
    {
        SDL_SetWindowRelativeMouseMode(mWindow, grab);
    }

    WGPUShaderModule Application::createShaderModule(const std::string &source)
    {
        WGPUShaderModuleWGSLDescriptor wgslDesc = {};
        wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslDesc.code = source.c_str();
        WGPUShaderModuleDescriptor desc = {};
        desc.nextInChain = (WGPUChainedStruct *)&wgslDesc;
        return wgpuDeviceCreateShaderModule(mDevice, &desc);
    }

    void Application::processKeyboardInput(const SDL_KeyboardEvent &keyEvent) {
        // No-op for now
    }

    void Application::cleanup()
    {
        wgpuRenderPipelineRelease(mRenderPipeline);
        wgpuPipelineLayoutRelease(mPipelineLayout);
        wgpuBindGroupLayoutRelease(mBindGroupLayout);
        wgpuShaderModuleRelease(mVertexShader);
        wgpuShaderModuleRelease(mFragmentShader);
        wgpuBufferRelease(mIndexBuffer);
        wgpuBufferRelease(mVertexBuffer);
        wgpuBindGroupRelease(mBindGroup);
        wgpuBufferRelease(mUniformBuffer);
        wgpuQueueRelease(mQueue);
        wgpuDeviceRelease(mDevice);
        wgpuAdapterRelease(mAdapter);
        wgpuSurfaceRelease(mSurface);
        wgpuInstanceRelease(mInstance);
        SDL_DestroyWindow(mWindow);
        SDL_Quit();
    }
}
