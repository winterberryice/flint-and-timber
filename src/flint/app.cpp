#include "app.hpp"
#include "world.hpp"
#include "player.hpp"
#include "constants.hpp"
#include "graphics/wgsl.h" // To be created
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>

namespace {

    struct AdapterRequestData {
        WGPUAdapter adapter = nullptr;
        bool done = false;
    };

    void on_adapter_received(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
        auto* data = static_cast<AdapterRequestData*>(userdata);
        if (status == WGPURequestAdapterStatus_Success) {
            data->adapter = adapter;
        } else {
            std::cerr << "Failed to get WGPU adapter: " << message << std::endl;
        }
        data->done = true;
    }

    struct DeviceRequestData {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    void on_device_received(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
        auto* data = static_cast<DeviceRequestData*>(userdata);
        if (status == WGPURequestDeviceStatus_Success) {
            data->device = device;
        } else {
            std::cerr << "Failed to get WGPU device: " << message << std::endl;
        }
        data->done = true;
    }

    void print_wgpu_error(WGPUErrorType type, const char* message, void*) {
        std::cerr << "WebGPU Error: " << type << " - " << message << std::endl;
    }
}

namespace flint {

    App::App() : m_player({8.0f, 20.0f, 8.0f}, -90.0f, 0.0f, 0.1f) {}

    bool App::Initialize(int width, int height) {
        m_windowWidth = width;
        m_windowHeight = height;

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        m_window = SDL_CreateWindow("Flint", width, height, SDL_WINDOW_RESIZABLE);
        if (!m_window) {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }

        m_instance = wgpuCreateInstance(nullptr);
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);

        AdapterRequestData adapter_data;
        WGPURequestAdapterOptions adapter_opts = {};
        adapter_opts.compatibleSurface = m_surface;
        wgpuInstanceRequestAdapter(m_instance, &adapter_opts, on_adapter_received, &adapter_data);

        // This is a simplification. A real app would have a proper event loop here.
        while (!adapter_data.done) {
            // A single call to process events. In a real app, you'd integrate this
            // with your main event loop.
        }
        m_adapter = adapter_data.adapter;


        DeviceRequestData device_data;
        WGPUDeviceDescriptor device_desc = {};
        device_desc.label = "Primary Device";
        wgpuAdapterRequestDevice(m_adapter, &device_desc, on_device_received, &device_data);
        while (!device_data.done) {
        }
        m_device = device_data.device;
        wgpuDeviceSetUncapturedErrorCallback(m_device, print_wgpu_error, nullptr);


        m_queue = wgpuDeviceGetQueue(m_device);

        WGPUSurfaceCapabilities caps;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &caps);
        m_surfaceFormat = caps.formats[0];

        WGPUSurfaceConfiguration config = {};
        config.device = m_device;
        config.format = m_surfaceFormat;
        config.width = width;
        config.height = height;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = WGPUPresentMode_Fifo;
        wgpuSurfaceConfigure(m_surface, &config);

        // Create Shaders
        WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
        wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgsl_desc.code = graphics::SHADER_WGSL;

        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = (WGPUChainedStruct*)&wgsl_desc;
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(m_device, &shader_desc);

        // Create Uniform Bind Group Layout
        WGPUBindGroupLayoutEntry bgl_entry = {};
        bgl_entry.binding = 0;
        bgl_entry.visibility = WGPUShaderStage_Vertex;
        bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;

        WGPUBindGroupLayoutDescriptor bgl_desc = {};
        bgl_desc.entryCount = 1;
        bgl_desc.entries = &bgl_entry;
        m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bgl_desc);

        // Create Pipeline Layout
        WGPUPipelineLayoutDescriptor layout_desc = {};
        layout_desc.bindGroupLayoutCount = 1;
        layout_desc.bindGroupLayouts = &m_bindGroupLayout;
        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(m_device, &layout_desc);

        // Create Depth Buffer
        WGPUTextureDescriptor depth_texture_desc = {};
        depth_texture_desc.dimension = WGPUTextureDimension_2D;
        depth_texture_desc.format = WGPUTextureFormat_Depth32Float;
        depth_texture_desc.size = { (uint32_t)width, (uint32_t)height, 1 };
        depth_texture_desc.usage = WGPUTextureUsage_RenderAttachment;
        depth_texture_desc.mipLevelCount = 1;
        depth_texture_desc.sampleCount = 1;
        m_depthTexture = wgpuDeviceCreateTexture(m_device, &depth_texture_desc);
        m_depthTextureView = wgpuTextureCreateView(m_depthTexture, nullptr);

        // Create Render Pipeline
        WGPURenderPipelineDescriptor pipeline_desc = {};
        pipeline_desc.layout = pipeline_layout;
        pipeline_desc.vertex.module = shader_module;
        pipeline_desc.vertex.entryPoint = "vs_main";

        WGPUVertexAttribute vert_attrs[3] = {};
        vert_attrs[0].format = WGPUVertexFormat_Float32x3; // position
        vert_attrs[0].offset = 0;
        vert_attrs[0].shaderLocation = 0;
        vert_attrs[1].format = WGPUVertexFormat_Float32x3; // color
        vert_attrs[1].offset = sizeof(glm::vec3);
        vert_attrs[1].shaderLocation = 1;
        vert_attrs[2].format = WGPUVertexFormat_Uint32; // sky_light
        vert_attrs[2].offset = 2 * sizeof(glm::vec3);
        vert_attrs[2].shaderLocation = 2;

        WGPUVertexBufferLayout vb_layout = {};
        vb_layout.arrayStride = sizeof(graphics::Vertex);
        vb_layout.attributeCount = 3;
        vb_layout.attributes = vert_attrs;
        pipeline_desc.vertex.bufferCount = 1;
        pipeline_desc.vertex.buffers = &vb_layout;

        WGPUColorTargetState color_target = {};
        color_target.format = m_surfaceFormat;
        color_target.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragment_state = {};
        fragment_state.module = shader_module;
        fragment_state.entryPoint = "fs_main";
        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target;
        pipeline_desc.fragment = &fragment_state;

        pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipeline_desc.primitive.cullMode = WGPUCullMode_Back;

        m_depthStencilState.format = WGPUTextureFormat_Depth32Float;
        m_depthStencilState.depthWriteEnabled = true;
        m_depthStencilState.depthCompare = WGPUCompareFunction_Less;
        pipeline_desc.depthStencil = &m_depthStencilState;

        m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipeline_desc);

        // Create Uniform Buffer
        WGPUBufferDescriptor uniform_buffer_desc = {};
        uniform_buffer_desc.size = sizeof(glm::mat4);
        uniform_buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniform_buffer_desc);

        WGPUBindGroupEntry bg_entry = {};
        bg_entry.binding = 0;
        bg_entry.buffer = m_uniformBuffer;
        bg_entry.size = sizeof(glm::mat4);

        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout = m_bindGroupLayout;
        bg_desc.entryCount = 1;
        bg_desc.entries = &bg_entry;
        m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &bg_desc);

        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuShaderModuleRelease(shader_module);

        m_running = true;
        return true;
    }

    void App::Run() {
        Uint64 last_time = SDL_GetPerformanceCounter();
        while (m_running) {
            Uint64 current_time = SDL_GetPerformanceCounter();
            float delta_time = (current_time - last_time) / (float)SDL_GetPerformanceFrequency();
            last_time = current_time;

            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) m_running = false;

                m_player.handle_input(e);

                if (e.type == SDL_EVENT_MOUSE_MOTION) {
                    m_player.process_mouse_movement(e.motion.xrel, e.motion.yrel);
                }
            }

            update(delta_time);
            render();
        }
    }

    void App::update(float dt) {
        m_player.update(dt, m_world);
        load_chunks();
    }

    void App::load_chunks() {
        int render_distance = 2;
        glm::ivec2 player_chunk_pos(
            floor(m_player.get_position().x / CHUNK_WIDTH),
            floor(m_player.get_position().z / CHUNK_DEPTH)
        );

        m_active_chunk_coords.clear();
        for (int x = -render_distance; x <= render_distance; ++x) {
            for (int z = -render_distance; z <= render_distance; ++z) {
                glm::ivec2 chunk_pos = player_chunk_pos + glm::ivec2(x, z);
                m_active_chunk_coords.push_back(chunk_pos);
                if (m_chunk_render_data.find(chunk_pos) == m_chunk_render_data.end()) {
                    build_chunk_mesh(chunk_pos.x, chunk_pos.y);
                }
            }
        }
    }

    void App::build_chunk_mesh(int chunk_x, int chunk_z) {
        Chunk* chunk = m_world.get_or_create_chunk(chunk_x, chunk_z);
        if (!chunk) return;

        // Only build mesh if the chunk is fully generated (for simplicity, we assume it is)
        graphics::ChunkMeshData mesh_data = chunk->build_mesh(&m_world);

        if (mesh_data.vertices.empty() || mesh_data.indices.empty()) {
            return; // Don't create buffers for empty meshes
        }

        ChunkRenderData render_data;
        render_data.index_count = mesh_data.indices.size();

        WGPUBufferDescriptor vb_desc = {};
        vb_desc.size = mesh_data.vertices.size() * sizeof(graphics::Vertex);
        vb_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        render_data.vertex_buffer = wgpuDeviceCreateBuffer(m_device, &vb_desc);
        wgpuQueueWriteBuffer(m_queue, render_data.vertex_buffer, 0, mesh_data.vertices.data(), vb_desc.size);

        WGPUBufferDescriptor ib_desc = {};
        ib_desc.size = mesh_data.indices.size() * sizeof(uint16_t);
        ib_desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        render_data.index_buffer = wgpuDeviceCreateBuffer(m_device, &ib_desc);
        wgpuQueueWriteBuffer(m_queue, render_data.index_buffer, 0, mesh_data.indices.data(), ib_desc.size);

        m_chunk_render_data[chunk->get_coord()] = render_data;
    }

    void App::render() {
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);
        if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
            if (surface_texture.texture) wgpuTextureRelease(surface_texture.texture);
            return;
        }

        glm::mat4 view = m_player.get_view_matrix();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)m_windowWidth / (float)m_windowHeight, 0.1f, 1000.0f);
        glm::mat4 view_proj = proj * view;
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &view_proj, sizeof(glm::mat4));

        WGPUTextureView frame_buffer_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

        WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view = frame_buffer_view;
        color_attachment.loadOp = WGPULoadOp_Clear;
        color_attachment.storeOp = WGPUStoreOp_Store;
        color_attachment.clearValue = {0.1, 0.2, 0.3, 1.0};

        WGPURenderPassDepthStencilAttachment depth_attachment = {};
        depth_attachment.view = m_depthTextureView;
        depth_attachment.depthClearValue = 1.0f;
        depth_attachment.depthLoadOp = WGPULoadOp_Clear;
        depth_attachment.depthStoreOp = WGPUStoreOp_Store;

        WGPURenderPassDescriptor pass_desc = {};
        pass_desc.colorAttachmentCount = 1;
        pass_desc.colorAttachments = &color_attachment;
        pass_desc.depthStencilAttachment = &depth_attachment;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);
        wgpuRenderPassEncoderSetPipeline(pass, m_renderPipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, m_bindGroup, 0, nullptr);

        for (const auto& coord : m_active_chunk_coords) {
            auto it = m_chunk_render_data.find(coord);
            if (it != m_chunk_render_data.end()) {
                const auto& render_data = it->second;
                if (render_data.index_count > 0) {
                    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, render_data.vertex_buffer, 0, WGPU_WHOLE_SIZE);
                    wgpuRenderPassEncoderSetIndexBuffer(pass, render_data.index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
                    wgpuRenderPassEncoderDrawIndexed(pass, render_data.index_count, 1, 0, 0, 0);
                }
            }
        }

        wgpuRenderPassEncoderEnd(pass);
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_queue, 1, &commands);
        wgpuSurfacePresent(m_surface);

        wgpuTextureViewRelease(frame_buffer_view);
        wgpuTextureRelease(surface_texture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandBufferRelease(commands);
    }

    void App::Terminate() {
        for (auto& pair : m_chunk_render_data) {
            pair.second.cleanup();
        }

        wgpuBufferRelease(m_uniformBuffer);
        wgpuBindGroupRelease(m_bindGroup);
        wgpuBindGroupLayoutRelease(m_bindGroupLayout);
        wgpuTextureViewRelease(m_depthTextureView);
        wgpuTextureRelease(m_depthTexture);
        wgpuRenderPipelineRelease(m_renderPipeline);
        wgpuSurfaceRelease(m_surface);
        wgpuDeviceRelease(m_device);
        wgpuAdapterRelease(m_adapter);
        wgpuInstanceRelease(m_instance);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
}
