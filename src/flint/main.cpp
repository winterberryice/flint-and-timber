#include "main.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <future>
#include <chrono>
#include "debug.h"

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

    // --- Application Class Implementation ---

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
        initSdl();
        initWebGpu();
        initCameraAndChunk();
        initPipeline(); // Must be after camera and chunk buffers are created
        setMouseGrab(true);
    }

    void Application::initSdl()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error("Could not initialize SDL");

        printVideoSystemInfo();

        int width = 1280, height = 720;
        mSize = {width, height};

        mWindow = SDL_CreateWindow("Flint (C++)", width, height, SDL_WINDOW_RESIZABLE);
        if (!mWindow)
            throw std::runtime_error("Could not create SDL window");

        printDetailedVideoInfo(mWindow);
    }

    void Application::initWebGpu()
    {
        mInstance = wgpuCreateInstance(nullptr);
        if (!mInstance)
            throw std::runtime_error("Could not create WGPU instance");

        mSurface = createSurface(mInstance, mWindow);

        getWgpuDevice();
        mQueue = wgpuDeviceGetQueue(mDevice);
        initSwapChain();
        // NOTE: No depth texture
    }

    void Application::getWgpuDevice()
    {
        // Request adapter
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.compatibleSurface = mSurface;

        AdapterRequestData adapterData;
        WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
        adapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        adapterCallbackInfo.callback = OnAdapterReceived;
        adapterCallbackInfo.userdata1 = &adapterData;

        wgpuInstanceRequestAdapter(mInstance, &adapterOptions, adapterCallbackInfo);

        while (!adapterData.done)
        {
            wgpuInstanceProcessEvents(mInstance);
        }

        if (!adapterData.adapter)
        {
            throw std::runtime_error("Failed to get WGPU adapter.");
        }
        mAdapter = adapterData.adapter;

        // Request device
        DeviceRequestData deviceData;
        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;

        wgpuAdapterRequestDevice(mAdapter, nullptr, deviceCallbackInfo);

        while (!deviceData.done)
        {
            wgpuInstanceProcessEvents(mInstance);
        }

        if (!deviceData.device)
        {
            throw std::runtime_error("Failed to get WGPU device.");
        }
        mDevice = deviceData.device;
    }

    void Application::initSwapChain()
    {
        WGPUSurfaceCapabilities caps;
        wgpuSurfaceGetCapabilities(mSurface, mAdapter, &caps);

        mSurfaceConfig = {};
        mSurfaceConfig.device = mDevice;
        mSurfaceConfig.format = caps.formats[0];
        mSurfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        mSurfaceConfig.width = mSize.x;
        mSurfaceConfig.height = mSize.y;
        mSurfaceConfig.presentMode = WGPUPresentMode_Fifo;
        mSurfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(mSurface, &mSurfaceConfig);
    }

    void Application::initCameraAndChunk()
    {
        // Equivalent to camera setup in `State::new`
        mCamera = Camera(
            {CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT * 0.75f, CHUNK_DEPTH * 2.0f}, // eye
            {CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f, CHUNK_DEPTH / 2.0f},  // target
            {0.0f, 1.0f, 0.0f},                                             // up
            (float)mSize.x / (float)mSize.y,                                // aspect
            45.0f,                                                          // fovy
            0.1f,                                                           // znear
            1000.0f                                                         // zfar
        );
        mCameraController = CameraController(15.0f, 0.003f);

        WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.label = makeStringView("Camera Buffer"); //"Camera Buffer";
        buffer_desc.size = sizeof(CameraUniform);
        buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        mCameraBuffer = wgpuDeviceCreateBuffer(mDevice, &buffer_desc);

        createHardcodedMesh();
    }

    void Application::initPipeline()
    {
        // Create Shader Modules
        mVertexShader = createShaderModule(VERTEX_SHADER_SOURCE);
        mFragmentShader = createShaderModule(FRAGMENT_SHADER_SOURCE);

        // Create Bind Group Layout
        WGPUBindGroupLayoutEntry bindingLayout = {};
        bindingLayout.binding = 0;
        bindingLayout.visibility = WGPUShaderStage_Vertex;
        bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayout.buffer.minBindingSize = sizeof(CameraUniform);

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &bindingLayout;
        mCameraBindGroupLayout = wgpuDeviceCreateBindGroupLayout(mDevice, &bindGroupLayoutDesc);

        // Create Pipeline Layout
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &mCameraBindGroupLayout;
        mPipelineLayout = wgpuDeviceCreatePipelineLayout(mDevice, &pipelineLayoutDesc);

        // Define Vertex Buffer Layout
        WGPUVertexBufferLayout vertexBufferLayout = Vertex::getLayout();

        // Create Render Pipeline
        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.layout = mPipelineLayout;

        // Vertex State
        pipelineDescriptor.vertex.module = mVertexShader;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        // Fragment State
        WGPUFragmentState fragmentState = {};
        fragmentState.module = mFragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");

        WGPUColorTargetState colorTarget = {};
        colorTarget.format = mSurfaceConfig.format;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive State
        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;

        // Other state
        pipelineDescriptor.depthStencil = nullptr;
        pipelineDescriptor.multisample.count = 1;

        mRenderPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipelineDescriptor);

        // Create Bind Group (AFTER pipeline creation, to match user's example)
        WGPUBindGroupEntry binding = {};
        binding.binding = 0;
        binding.buffer = mCameraBuffer;
        binding.offset = 0;
        binding.size = sizeof(CameraUniform);

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = mCameraBindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;
        mCameraBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bindGroupDesc);
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
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                mIsRunning = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                resize(event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (!mMouseGrabbed)
                    setMouseGrab(true);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (mMouseGrabbed)
                    mCameraController.processMouseMovement(event.motion.xrel, event.motion.yrel);
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                processKeyboardInput(event.key);
                break;
            }
        }
    }

    void Application::processKeyboardInput(const SDL_KeyboardEvent &keyEvent)
    {
        bool is_pressed = (keyEvent.type == SDL_EVENT_KEY_DOWN);

        switch (keyEvent.key)
        {
        case SDLK_W:
        case SDLK_UP:
            mCameraController.movement.is_forward_pressed = is_pressed;
            break;
        case SDLK_S:
        case SDLK_DOWN:
            mCameraController.movement.is_backward_pressed = is_pressed;
            break;
        case SDLK_A:
        case SDLK_LEFT:
            mCameraController.movement.is_left_pressed = is_pressed;
            break;
        case SDLK_D:
        case SDLK_RIGHT:
            mCameraController.movement.is_right_pressed = is_pressed;
            break;
        case SDLK_SPACE:
            mCameraController.movement.is_up_pressed = is_pressed;
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            mCameraController.movement.is_down_pressed = is_pressed;
            break;
        case SDLK_ESCAPE:
            if (is_pressed)
            {
                if (mMouseGrabbed)
                    setMouseGrab(false);
                else
                    mIsRunning = false;
            }
            break;
        }
    }

    void Application::update()
    {
        // Use a fixed delta time for simplicity, like in the Rust code
        float dt = 1.0f / 60.0f;
        mCameraController.updateCamera(mCamera, dt);
        mCameraUniform.updateViewProj(mCamera);
        wgpuQueueWriteBuffer(mQueue, mCameraBuffer, 0, &mCameraUniform, sizeof(CameraUniform));
    }

    void Application::render()
    {
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(mSurface, &surface_texture);

        if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
        {
            if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Lost)
                initSwapChain();
            return;
        }

        WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, nullptr);

        WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view = view;
        color_attachment.loadOp = WGPULoadOp_Clear;
        color_attachment.storeOp = WGPUStoreOp_Store;
        color_attachment.clearValue = {0.1, 0.2, 0.3, 1.0};

        WGPURenderPassDescriptor pass_desc = {};
        pass_desc.colorAttachmentCount = 1;
        pass_desc.colorAttachments = &color_attachment;
        pass_desc.depthStencilAttachment = nullptr;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);
        wgpuRenderPassEncoderSetPipeline(pass, mRenderPipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, mCameraBindGroup, 0, nullptr);

        if (mChunkVertexBuffer && mChunkIndexBuffer && mNumChunkIndices > 0)
        {
            wgpuRenderPassEncoderSetVertexBuffer(pass, 0, mChunkVertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(pass, mChunkIndexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderDrawIndexed(pass, mNumChunkIndices, 1, 0, 0, 0);
        }

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(mQueue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        wgpuSurfacePresent(mSurface);

        wgpuTextureViewRelease(view);
        wgpuTextureRelease(surface_texture.texture);
    }

    void Application::buildChunkMesh()
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        uint16_t current_vertex_offset = 0;

        const std::array<std::pair<CubeGeometry::Face, glm::ivec3>, 6> face_definitions = {{{CubeGeometry::Face::Front, {0, 0, -1}}, {CubeGeometry::Face::Back, {0, 0, 1}}, {CubeGeometry::Face::Right, {1, 0, 0}}, {CubeGeometry::Face::Left, {-1, 0, 0}}, {CubeGeometry::Face::Top, {0, 1, 0}}, {CubeGeometry::Face::Bottom, {0, -1, 0}}}};

        for (size_t x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (size_t y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (size_t z = 0; z < CHUNK_DEPTH; ++z)
                {
                    const Block *block = mChunk.getBlock(x, y, z);
                    if (block && block->isSolid())
                    {
                        glm::vec3 block_color;
                        switch (block->type)
                        {
                        case BlockType::Dirt:
                            block_color = {0.5f, 0.25f, 0.05f};
                            break;
                        case BlockType::Grass:
                            block_color = {0.0f, 0.8f, 0.1f};
                            break;
                        default:
                            continue;
                        }
                        glm::vec3 block_center(x + 0.5f, y + 0.5f, z + 0.5f);

                        for (const auto &[face_type, offset] : face_definitions)
                        {
                            glm::ivec3 neighbor_pos = glm::ivec3(x, y, z) + offset;
                            const Block *neighbor = mChunk.getBlock(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z);

                            if (!neighbor || !neighbor->isSolid())
                            {
                                std::vector<Vertex> face_verts = CubeGeometry::getFaceVertices(face_type);
                                for (const auto &v_template : face_verts)
                                {
                                    vertices.push_back({block_center + v_template.position, block_color});
                                }

                                const auto &local_indices = CubeGeometry::getLocalFaceIndices();
                                for (uint16_t local_idx : local_indices)
                                {
                                    indices.push_back(current_vertex_offset + local_idx);
                                }
                                current_vertex_offset += face_verts.size();
                            }
                        }
                    }
                }
            }
        }

        if (mChunkVertexBuffer)
            wgpuBufferRelease(mChunkVertexBuffer);
        if (mChunkIndexBuffer)
            wgpuBufferRelease(mChunkIndexBuffer);

        if (!vertices.empty())
        {
            WGPUBufferDescriptor vb_desc = {};
            vb_desc.size = vertices.size() * sizeof(Vertex);
            vb_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            mChunkVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &vb_desc);
            wgpuQueueWriteBuffer(mQueue, mChunkVertexBuffer, 0, vertices.data(), vb_desc.size);

            WGPUBufferDescriptor ib_desc = {};
            ib_desc.size = indices.size() * sizeof(uint16_t);
            ib_desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            mChunkIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &ib_desc);
            wgpuQueueWriteBuffer(mQueue, mChunkIndexBuffer, 0, indices.data(), ib_desc.size);
            mNumChunkIndices = indices.size();
        }
        else
        {
            mNumChunkIndices = 0;
            mChunkVertexBuffer = nullptr;
            mChunkIndexBuffer = nullptr;
        }
    }

    void Application::resize(int new_width, int new_height)
    {
        if (new_width > 0 && new_height > 0)
        {
            mSize = {(unsigned int)new_width, (unsigned int)new_height};
            mCamera.aspect = (float)mSize.x / (float)mSize.y;
            initSwapChain();
        }
    }

    void Application::setMouseGrab(bool grab)
    {
        SDL_SetWindowRelativeMouseMode(mWindow, grab);
        SDL_SetWindowMouseGrab(mWindow, grab);
        mMouseGrabbed = grab;
    }

    WGPUShaderModule Application::createShaderModule(const std::string &source)
    {
        WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
        wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgsl_desc.code = makeStringView(source.c_str());
        WGPUShaderModuleDescriptor desc = {};
        desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgsl_desc);
        return wgpuDeviceCreateShaderModule(mDevice, &desc);
    }

    void Application::cleanup()
    {
        wgpuRenderPipelineRelease(mRenderPipeline);
        wgpuPipelineLayoutRelease(mPipelineLayout);
        wgpuBindGroupLayoutRelease(mCameraBindGroupLayout);
        wgpuShaderModuleRelease(mVertexShader);
        wgpuShaderModuleRelease(mFragmentShader);
        wgpuBufferRelease(mChunkIndexBuffer);
        wgpuBufferRelease(mChunkVertexBuffer);
        wgpuBindGroupRelease(mCameraBindGroup);
        wgpuBufferRelease(mCameraBuffer);
        wgpuQueueRelease(mQueue);
        wgpuDeviceRelease(mDevice);
        wgpuAdapterRelease(mAdapter);
        wgpuSurfaceRelease(mSurface);
        wgpuInstanceRelease(mInstance);
        SDL_DestroyWindow(mWindow);
        SDL_Quit();
    }

} // namespace flint
