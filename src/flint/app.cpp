#include "app.hpp"
#include "world.hpp"
#include "player.hpp"
#include "input.hpp"
#include "raycast.hpp"
#include "ui/crosshair.hpp"
#include "physics.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <fstream>
#include <sstream>

// Forward declarations for helper functions
namespace
{
    void printVideoSystemInfo();
    void printDetailedVideoInfo(SDL_Window *window);
    WGPUStringView makeStringView(const char *str);

    struct AdapterRequestData
    {
        WGPUAdapter adapter = nullptr;
        bool done = false;
    };
    void OnAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2);

    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };
    void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2);

    std::string read_shader_file(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void printVideoSystemInfo()
    {
        const char *videoDriver = SDL_GetCurrentVideoDriver();
        if (videoDriver)
        {
            printf("Current Video Driver: %s\n", videoDriver);
        }
    }

    void printDetailedVideoInfo(SDL_Window *window)
    {
        // Placeholder for detailed info
    }
}

namespace flint
{
    // A new helper function for creating the depth texture
    void create_depth_texture(WGPUDevice device, uint32_t width, uint32_t height, WGPUTexture* depth_texture, WGPUTextureView* depth_texture_view) {
        if (*depth_texture) wgpuTextureRelease(*depth_texture);
        if (*depth_texture_view) wgpuTextureViewRelease(*depth_texture_view);

        WGPUTextureDescriptor depth_texture_desc = {};
        depth_texture_desc.size = {width, height, 1};
        depth_texture_desc.mipLevelCount = 1;
        depth_texture_desc.sampleCount = 1;
        depth_texture_desc.dimension = WGPUTextureDimension_2D;
        depth_texture_desc.format = WGPUTextureFormat_Depth24PlusStencil8;
        depth_texture_desc.usage = WGPUTextureUsage_RenderAttachment;

        *depth_texture = wgpuDeviceCreateTexture(device, &depth_texture_desc);

        WGPUTextureViewDescriptor view_desc = {};
        view_desc.format = WGPUTextureFormat_Depth24PlusStencil8;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.aspect = WGPUTextureAspect_All;
        *depth_texture_view = wgpuTextureCreateView(*depth_texture, &view_desc);
    }


    App::App()
        : m_player(
              glm::vec3(8.0f, 20.0f, 8.0f),
              -90.0f,
              0.0f,
              0.1f)
    {
    }

    static Uint64 s_now = 0;
    static Uint64 s_last = 0;
    static float s_delta_time = 0.0f;
    static bool s_mouse_grabbed = false;

    bool App::Initialize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        printVideoSystemInfo();
        m_window = SDL_CreateWindow("Flint", m_windowWidth, m_windowHeight, 0);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }
        printDetailedVideoInfo(m_window);

        // --- WebGPU Initialization ---
        m_instance = wgpuCreateInstance(nullptr);
        if (!m_instance) return false;

        // Request Adapter
        AdapterRequestData adapterData;
        WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
        adapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        adapterCallbackInfo.callback = OnAdapterReceived;
        adapterCallbackInfo.userdata1 = &adapterData;
        wgpuInstanceRequestAdapter(m_instance, nullptr, adapterCallbackInfo);
        while (!adapterData.done) { wgpuInstanceProcessEvents(m_instance); }
        m_adapter = adapterData.adapter;
        if (!m_adapter) return false;

        // Request Device
        DeviceRequestData deviceData;
        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;
        wgpuAdapterRequestDevice(m_adapter, nullptr, deviceCallbackInfo);
        while (!deviceData.done) { wgpuInstanceProcessEvents(m_instance); }
        m_device = deviceData.device;
        if (!m_device) return false;

        m_queue = wgpuDeviceGetQueue(m_device);

        // Create Surface and configure
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);
        WGPUSurfaceCapabilities surfaceCapabilities;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &surfaceCapabilities);
        m_surfaceFormat = surfaceCapabilities.formats[0];
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.device = m_device;
        surfaceConfig.format = m_surfaceFormat;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        wgpuSurfaceCapabilitiesFreeMembers(surfaceCapabilities);


        // --- Load Assets ---
        m_texture_atlas = graphics::Texture::load_from_file(m_device, m_queue, "assets/textures/dirt.png");
        if (m_texture_atlas.get_view() == nullptr) {
            std::cerr << "Failed to load texture atlas." << std::endl;
            return false;
        }


        // --- Create Shaders ---
        std::string shaderSource = read_shader_file("assets/shaders/world.wgsl");
        if (shaderSource.empty()) {
            return false;
        }

        WGPUShaderModuleWGSLDescriptor wgslDesc = {};
        wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslDesc.code = shaderSource.c_str();
        WGPUShaderModuleDescriptor shaderDesc = {};
        shaderDesc.nextInChain = &wgslDesc.chain;
        m_vertexShader = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);
        m_fragmentShader = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);


        // --- Create Bind Group Layouts ---
        // Camera Bind Group Layout
        WGPUBindGroupLayoutEntry cameraLayoutEntry = {};
        cameraLayoutEntry.binding = 0;
        cameraLayoutEntry.visibility = WGPUShaderStage_Vertex;
        cameraLayoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
        WGPUBindGroupLayoutDescriptor cameraBindGroupLayoutDesc = {};
        cameraBindGroupLayoutDesc.entryCount = 1;
        cameraBindGroupLayoutDesc.entries = &cameraLayoutEntry;
        m_camera_bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &cameraBindGroupLayoutDesc);

        // Texture Bind Group Layout
        WGPUBindGroupLayoutEntry textureLayoutEntry[2] = {};
        textureLayoutEntry[0].binding = 0;
        textureLayoutEntry[0].visibility = WGPUShaderStage_Fragment;
        textureLayoutEntry[0].texture.sampleType = WGPUTextureSampleType_Float;
        textureLayoutEntry[0].texture.viewDimension = WGPUTextureViewDimension_2D;
        textureLayoutEntry[1].binding = 1;
        textureLayoutEntry[1].visibility = WGPUShaderStage_Fragment;
        textureLayoutEntry[1].sampler.type = WGPUSamplerBindingType_Filtering;
        WGPUBindGroupLayoutDescriptor textureBindGroupLayoutDesc = {};
        textureBindGroupLayoutDesc.entryCount = 2;
        textureBindGroupLayoutDesc.entries = textureLayoutEntry;
        m_texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &textureBindGroupLayoutDesc);


        // --- Create Render Pipeline ---
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        std::vector<WGPUBindGroupLayout> bindGroupLayouts = {m_camera_bind_group_layout, m_texture_bind_group_layout};
        pipelineLayoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
        pipelineLayoutDesc.bindGroupLayouts = bindGroupLayouts.data();
        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);

        WGPUVertexAttribute vertexAttribs[4] = {};
        vertexAttribs[0].format = WGPUVertexFormat_Float32x3; // position
        vertexAttribs[0].offset = offsetof(graphics::Vertex, position);
        vertexAttribs[0].shaderLocation = 0;
        vertexAttribs[1].format = WGPUVertexFormat_Float32x2; // tex_coords
        vertexAttribs[1].offset = offsetof(graphics::Vertex, tex_coords);
        vertexAttribs[1].shaderLocation = 1;
        vertexAttribs[2].format = WGPUVertexFormat_Float32x3; // normal
        vertexAttribs[2].offset = offsetof(graphics::Vertex, normal);
        vertexAttribs[2].shaderLocation = 2;
        vertexAttribs[3].format = WGPUVertexFormat_Float32; // light
        vertexAttribs[3].offset = offsetof(graphics::Vertex, light);
        vertexAttribs[3].shaderLocation = 3;

        WGPUVertexBufferLayout vertexBufferLayout = {};
        vertexBufferLayout.arrayStride = sizeof(graphics::Vertex);
        vertexBufferLayout.attributeCount = 4;
        vertexBufferLayout.attributes = vertexAttribs;

        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.layout = pipelineLayout;
        pipelineDesc.vertex.module = m_vertexShader;
        pipelineDesc.vertex.entryPoint = makeStringView("vs_main");
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        WGPUFragmentState fragmentState = {};
        fragmentState.module = m_fragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = m_surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.primitive.cullMode = WGPUCullMode_Back;

        // Enable depth testing
        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;
        depthStencilState.depthWriteEnabled = true;
        depthStencilState.depthCompare = WGPUCompareFunction_Less;
        pipelineDesc.depthStencil = &depthStencilState;

        m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDesc);
        wgpuPipelineLayoutRelease(pipelineLayout);


        // --- Create Buffers and Bind Groups ---
        // Camera uniform buffer
        WGPUBufferDescriptor uniformBufferDesc = {};
        uniformBufferDesc.size = sizeof(glm::mat4);
        uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        m_camera_uniform_buffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);

        // Camera bind group
        WGPUBindGroupEntry cameraBinding = {};
        cameraBinding.binding = 0;
        cameraBinding.buffer = m_camera_uniform_buffer;
        cameraBinding.size = sizeof(glm::mat4);
        WGPUBindGroupDescriptor cameraBindGroupDesc = {};
        cameraBindGroupDesc.layout = m_camera_bind_group_layout;
        cameraBindGroupDesc.entryCount = 1;
        cameraBindGroupDesc.entries = &cameraBinding;
        m_camera_bind_group = wgpuDeviceCreateBindGroup(m_device, &cameraBindGroupDesc);

        // Texture bind group
        WGPUBindGroupEntry textureBindings[2] = {};
        textureBindings[0].binding = 0;
        textureBindings[0].textureView = m_texture_atlas.get_view();
        textureBindings[1].binding = 1;
        textureBindings[1].sampler = m_texture_atlas.get_sampler();
        WGPUBindGroupDescriptor textureBindGroupDesc = {};
        textureBindGroupDesc.layout = m_texture_bind_group_layout;
        textureBindGroupDesc.entryCount = 2;
        textureBindGroupDesc.entries = textureBindings;
        m_texture_bind_group = wgpuDeviceCreateBindGroup(m_device, &textureBindGroupDesc);

        // --- Initialize Game Objects ---
        m_world.generate_initial_chunks(m_device);
        if (!m_crosshair.initialize(m_device, m_surfaceFormat, m_windowWidth, m_windowHeight))
        {
            std::cerr << "Failed to initialize crosshair!" << std::endl;
            return false;
        }

        create_depth_texture(m_device, m_windowWidth, m_windowHeight, &m_depth_texture, &m_depth_texture_view);

        m_running = true;
        return true;
    }

    void App::Run()
    {
        s_last = SDL_GetPerformanceCounter();
        while (m_running)
        {
            s_now = SDL_GetPerformanceCounter();
            s_delta_time = (s_now - s_last) / (float)SDL_GetPerformanceFrequency();
            s_last = s_now;

            m_input.clear_frame_state();
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                m_input.on_event(e);

                if (e.type == SDL_EVENT_QUIT) m_running = false;
                else if (e.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    m_windowWidth = e.window.data1;
                    m_windowHeight = e.window.data2;

                    WGPUSurfaceConfiguration surfaceConfig = {};
                    surfaceConfig.device = m_device;
                    surfaceConfig.format = m_surfaceFormat;
                    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
                    surfaceConfig.width = m_windowWidth;
                    surfaceConfig.height = m_windowHeight;
                    surfaceConfig.presentMode = WGPUPresentMode_Fifo;
                    surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
                    wgpuSurfaceConfigure(m_surface, &surfaceConfig);

                    create_depth_texture(m_device, m_windowWidth, m_windowHeight, &m_depth_texture, &m_depth_texture_view);
                    m_crosshair.resize(m_windowWidth, m_windowHeight, m_queue);
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    s_mouse_grabbed = !s_mouse_grabbed;
                    SDL_SetWindowRelativeMouseMode(m_window, s_mouse_grabbed);
                }
                if (s_mouse_grabbed) {
                    if (e.type == SDL_EVENT_MOUSE_MOTION) m_player.process_mouse_movement(e.motion.xrel, e.motion.yrel);
                }
            }

            if (s_mouse_grabbed) {
                m_player.handle_keyboard_input(m_input);
            }
            m_player.update(s_delta_time, m_world);
            m_raycast_result = cast_ray(m_player, m_world, 5.0f); // 5 blocks reach

            // Block interaction
            if (s_mouse_grabbed)
            {
                if (m_input.left_mouse_pressed_this_frame && m_raycast_result)
                {
                    m_world.set_block_at_world(m_device, m_raycast_result->block_position.x, m_raycast_result->block_position.y, m_raycast_result->block_position.z, Block(BlockType::Air));
                }
                else if (m_input.right_mouse_pressed_this_frame && m_raycast_result)
                {
                    glm::ivec3 place_pos = m_raycast_result->block_position;
                    switch (m_raycast_result->hit_face)
                    {
                    case BlockFace::PosX: place_pos.x++; break;
                    case BlockFace::NegX: place_pos.x--; break;
                    case BlockFace::PosY: place_pos.y++; break;
                    case BlockFace::NegY: place_pos.y--; break;
                    case BlockFace::PosZ: place_pos.z++; break;
                    case BlockFace::NegZ: place_pos.z--; break;
                    }
                    m_world.set_block_at_world(m_device, place_pos.x, place_pos.y, place_pos.z, Block(BlockType::Dirt));
                }
            }

            render();
        }
    }

    void App::render()
    {
        // Update camera matrix
        glm::mat4 viewProjectionMatrix = m_player.get_view_projection_matrix((float)m_windowWidth / (float)m_windowHeight);
        wgpuQueueWriteBuffer(m_queue, m_camera_uniform_buffer, 0, &viewProjectionMatrix, sizeof(glm::mat4));

        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) return;

        WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = textureView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.5f, 0.6f, 0.7f, 1.0f}; // Sky blue

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;

        WGPURenderPassDepthStencilAttachment depthAttachment = {};
        depthAttachment.view = m_depth_texture_view;
        depthAttachment.depthClearValue = 1.0f;
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthAttachment.depthReadOnly = false;
        depthAttachment.stencilClearValue = 0;
        depthAttachment.stencilLoadOp = WGPULoadOp_Undefined;
        depthAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
        depthAttachment.stencilReadOnly = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_camera_bind_group, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 1, m_texture_bind_group, 0, nullptr);

        m_world.render(renderPass);
        m_crosshair.render(renderPass);

        wgpuRenderPassEncoderEnd(renderPass);

        WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

        wgpuSurfacePresent(m_surface);

        wgpuTextureViewRelease(textureView);
        wgpuTextureRelease(surfaceTexture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuRenderPassEncoderRelease(renderPass);
        wgpuCommandBufferRelease(cmdBuffer);
    }

    void App::Terminate()
    {
        // Chunk and Texture are cleaned up by their destructors
        if (m_renderPipeline) wgpuRenderPipelineRelease(m_renderPipeline);
        if (m_vertexShader) wgpuShaderModuleRelease(m_vertexShader);
        if (m_fragmentShader) wgpuShaderModuleRelease(m_fragmentShader);
        if (m_camera_uniform_buffer) wgpuBufferRelease(m_camera_uniform_buffer);
        if (m_camera_bind_group) wgpuBindGroupRelease(m_camera_bind_group);
        if (m_camera_bind_group_layout) wgpuBindGroupLayoutRelease(m_camera_bind_group_layout);
        if (m_texture_bind_group) wgpuBindGroupRelease(m_texture_bind_group);
        if (m_texture_bind_group_layout) wgpuBindGroupLayoutRelease(m_texture_bind_group_layout);
        if (m_depth_texture) wgpuTextureRelease(m_depth_texture);
        if (m_depth_texture_view) wgpuTextureViewRelease(m_depth_texture_view);

        if (m_surface) wgpuSurfaceRelease(m_surface);
        if (m_queue) wgpuQueueRelease(m_queue);
        if (m_device) wgpuDeviceRelease(m_device);
        if (m_adapter) wgpuAdapterRelease(m_adapter);
        if (m_instance) wgpuInstanceRelease(m_instance);
        if (m_window) SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

} // namespace flint

// Helper function implementations
namespace
{
    void printVideoSystemInfo() { /* ... */ }
    void printDetailedVideoInfo(SDL_Window *window) { /* ... */ }
    WGPUStringView makeStringView(const char *str) { return WGPUStringView{ .data = str, .length = str ? strlen(str) : 0 }; }
    void OnAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2) {
        AdapterRequestData *data = static_cast<AdapterRequestData *>(userdata1);
        if (status == WGPURequestAdapterStatus_Success) data->adapter = adapter;
        else std::cerr << "Failed to get adapter: " << std::string(message.data, message.length) << std::endl;
        data->done = true;
    }
    void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2) {
        DeviceRequestData *data = static_cast<DeviceRequestData *>(userdata1);
        if (status == WGPURequestDeviceStatus_Success) data->device = device;
        else std::cerr << "Failed to get device: " << std::string(message.data, message.length) << std::endl;
        data->done = true;
    }
}
