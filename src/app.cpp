#include "app.h"
#include <iostream>
#include <vector>
#include <string>
#include "glm/gtc/matrix_transform.hpp"
// ifstream include
#include <fstream>
#include "flint_utils.h"

// TODO: This is a placeholder for a utility to get the Dawn device.
// In a real application, this would be handled more robustly.
namespace
{
    void PrintDeviceError(WGPUErrorType type, const char *message, void *)
    {
        std::cerr << "Dawn device error: " << type << " - " << message << std::endl;
    }

    // readFile helper function
    std::string readFile(const std::string &filename)
    {
        std::ifstream file(filename);
        std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
        return content;
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

    // --- ChunkRenderBuffers Destructor and Move Semantics ---
    ChunkRenderBuffers::~ChunkRenderBuffers()
    {
        if (vertex_buffer)
            wgpuBufferRelease(vertex_buffer);
        if (index_buffer)
            wgpuBufferRelease(index_buffer);
    }

    ChunkRenderBuffers::ChunkRenderBuffers(ChunkRenderBuffers &&other) noexcept
        : vertex_buffer(other.vertex_buffer), index_buffer(other.index_buffer), num_indices(other.num_indices)
    {
        other.vertex_buffer = nullptr;
        other.index_buffer = nullptr;
        other.num_indices = 0;
    }

    ChunkRenderBuffers &ChunkRenderBuffers::operator=(ChunkRenderBuffers &&other) noexcept
    {
        if (this != &other)
        {
            if (vertex_buffer)
                wgpuBufferRelease(vertex_buffer);
            if (index_buffer)
                wgpuBufferRelease(index_buffer);
            vertex_buffer = other.vertex_buffer;
            index_buffer = other.index_buffer;
            num_indices = other.num_indices;
            other.vertex_buffer = nullptr;
            other.index_buffer = nullptr;
            other.num_indices = 0;
        }
        return *this;
    }

    App::App(SDL_Window *_window) : window(_window)
    {
        // --- Initialize Dawn ---
        // TODO: This is a simplified Dawn setup. Error handling and configuration should be more robust.
        width = 1280; // default
        height = 720;
        SDL_GetWindowSize(window, (int *)&width, (int *)&height);

        instance = wgpuCreateInstance(nullptr);
        // new ================================================

        if (!instance)
        {
            std::cerr << "Failed to create WebGPU instance" << std::endl;
            throw std::runtime_error("Failed to create WebGPU instance");
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
        wgpuInstanceRequestAdapter(instance, &adapterOptions, callbackInfo);

        std::cout << "Processing events until adapter callback..." << std::endl;
        // Process events until callback is called
        int attempts = 0;
        while (!adapterData.done && attempts < 1000)
        {
            // Try to process pending events
            wgpuInstanceProcessEvents(instance);

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
            throw std::runtime_error("No suitable WebGPU adapter found");
        }

        adapter = adapterData.adapter;
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
        wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);

        std::cout << "Processing events until device callback..." << std::endl;
        attempts = 0;
        while (!deviceData.done && attempts < 1000)
        {
            wgpuInstanceProcessEvents(instance);
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
            throw std::runtime_error("Failed to create WebGPU device");
        }

        device = deviceData.device;
        std::cout << "WebGPU device created successfully" << std::endl;

        // Get the queue
        queue = wgpuDeviceGetQueue(device);
        std::cout << "WebGPU queue obtained" << std::endl;

        // Create surface
        surface = SDL_GetWGPUSurface(instance, window);
        if (!surface)
        {
            std::cerr << "Failed to create WebGPU surface" << std::endl;
            throw std::runtime_error("Failed to create WebGPU surface");
        }
        std::cout << "WebGPU surface created successfully" << std::endl;

        // Get supported surface formats
        WGPUSurfaceCapabilities surfaceCapabilities;
        wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCapabilities);

        // Use the first supported format (this is the preferred one)
        auto m_surfaceFormat = surfaceCapabilities.formats[0];
        std::cout << "Using surface format: " << m_surfaceFormat << std::endl;

        // Configure the surface
        config = {};
        config.nextInChain = nullptr;
        config.device = device;
        config.format = m_surfaceFormat; // Common format
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.width = width;
        config.height = height;
        config.presentMode = WGPUPresentMode_Fifo;
        config.alphaMode = WGPUCompositeAlphaMode_Auto;
        config.viewFormatCount = 0;
        config.viewFormats = nullptr;
        wgpuSurfaceCapabilitiesFreeMembers(surfaceCapabilities);
        //

        wgpuSurfaceConfigure(surface, &config);
        std::cout << "WebGPU surface configured" << std::endl;

        // --- Main Render Pipeline ---
        std::string shader_code = readFile("src/shader.wgsl");
        if (shader_code.empty())
        {
            throw std::runtime_error("Failed to read shader file: src/shader.wgsl or it was empty.");
        }

        WGPUShaderModuleWGSLDescriptor shader_wgsl = {};
        shader_wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
        shader_wgsl.code = flint_utils::makeStringView(shader_code.c_str());

        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = &shader_wgsl.chain;
        shader_desc.label = flint_utils::makeStringView("Main Shader Module");
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);
        if (!shader_module)
        {
            throw std::runtime_error("Failed to create shader module.");
        }

        // --- Create Bind Group Layouts ---
        // Camera BGL
        WGPUBindGroupLayoutEntry camera_bgl_entry = {};
        camera_bgl_entry.binding = 0;
        camera_bgl_entry.visibility = WGPUShaderStage_Vertex;
        camera_bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;
        camera_bgl_entry.buffer.minBindingSize = sizeof(CameraUniform);

        WGPUBindGroupLayoutDescriptor camera_bgl_desc = {};
        camera_bgl_desc.label = flint_utils::makeStringView("Camera Bind Group Layout");
        camera_bgl_desc.entryCount = 1;
        camera_bgl_desc.entries = &camera_bgl_entry;
        camera_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &camera_bgl_desc);
        if (!camera_bind_group_layout)
        {
            throw std::runtime_error("Failed to create camera bind group layout.");
        }

        // Texture BGL
        WGPUBindGroupLayoutEntry texture_bgl_entries[2] = {};
        texture_bgl_entries[0].binding = 0;
        texture_bgl_entries[0].visibility = WGPUShaderStage_Fragment;
        texture_bgl_entries[0].texture.sampleType = WGPUTextureSampleType_Float;
        texture_bgl_entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        texture_bgl_entries[1].binding = 1;
        texture_bgl_entries[1].visibility = WGPUShaderStage_Fragment;
        texture_bgl_entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor texture_bgl_desc = {};
        texture_bgl_desc.label = flint_utils::makeStringView("Texture Bind Group Layout");
        texture_bgl_desc.entryCount = 2;
        texture_bgl_desc.entries = texture_bgl_entries;
        texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &texture_bgl_desc);
        if (!texture_bind_group_layout)
        {
            throw std::runtime_error("Failed to create texture bind group layout.");
        }

        // --- Create Pipeline Layout ---
        std::vector<WGPUBindGroupLayout> bgls = {camera_bind_group_layout, texture_bind_group_layout};
        WGPUPipelineLayoutDescriptor layout_desc = {};
        layout_desc.label = flint_utils::makeStringView("Render Pipeline Layout");
        layout_desc.bindGroupLayoutCount = (uint32_t)bgls.size();
        layout_desc.bindGroupLayouts = bgls.data();
        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);
        if (!pipeline_layout)
        {
            throw std::runtime_error("Failed to create pipeline layout.");
        }

        // --- Create Render Pipelines ---

        // Shared Vertex State
        WGPUVertexState vertex_state = {};
        vertex_state.module = shader_module;
        vertex_state.entryPoint = flint_utils::makeStringView("vs_main");
        WGPUVertexBufferLayout vertex_buffer_layout = Vertex::get_layout();
        vertex_state.bufferCount = 1;
        vertex_state.buffers = &vertex_buffer_layout;

        // Shared Depth/Stencil State
        WGPUDepthStencilState depth_stencil_state = {};
        depth_stencil_state.format = WGPUTextureFormat_Depth32Float;
        depth_stencil_state.depthWriteEnabled = WGPUOptionalBool_True;
        depth_stencil_state.depthCompare = WGPUCompareFunction_Less;

        // Shared Fragment State for both pipelines
        WGPUFragmentState fragment_state = {};
        fragment_state.module = shader_module;
        fragment_state.entryPoint = flint_utils::makeStringView("fs_main");
        fragment_state.targetCount = 1;
        // The rest will be configured per-pipeline

        // --- Opaque Render Pipeline ---
        WGPURenderPipelineDescriptor opaque_pipeline_desc = {};
        opaque_pipeline_desc.label = flint_utils::makeStringView("Opaque Render Pipeline");
        opaque_pipeline_desc.layout = pipeline_layout;
        opaque_pipeline_desc.vertex = vertex_state;

        WGPUBlendState opaque_blend_state = {};
        opaque_blend_state.color = {WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha};
        opaque_blend_state.alpha = {WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};

        WGPUColorTargetState opaque_color_target = {};
        opaque_color_target.format = config.format;
        opaque_color_target.blend = &opaque_blend_state;
        opaque_color_target.writeMask = WGPUColorWriteMask_All;

        fragment_state.targets = &opaque_color_target;
        opaque_pipeline_desc.fragment = &fragment_state;

        opaque_pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        opaque_pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
        opaque_pipeline_desc.primitive.cullMode = WGPUCullMode_Back;

        opaque_pipeline_desc.depthStencil = &depth_stencil_state;

        opaque_pipeline_desc.multisample.count = 1;
        opaque_pipeline_desc.multisample.mask = 0xFFFFFFFF;

        render_pipeline = wgpuDeviceCreateRenderPipeline(device, &opaque_pipeline_desc);
        if (!render_pipeline)
        {
            throw std::runtime_error("Failed to create opaque render pipeline.");
        }

        // --- Transparent Render Pipeline ---
        WGPURenderPipelineDescriptor transparent_pipeline_desc = {};
        transparent_pipeline_desc.label = flint_utils::makeStringView("Transparent Render Pipeline");
        transparent_pipeline_desc.layout = pipeline_layout;
        transparent_pipeline_desc.vertex = vertex_state; // Reuse vertex state

        // Different blend and cull states for transparent objects
        WGPUColorTargetState transparent_color_target = {};
        transparent_color_target.format = config.format;
        transparent_color_target.blend = nullptr; // No blending for now, as in Rust code
        transparent_color_target.writeMask = WGPUColorWriteMask_All;

        fragment_state.targets = &transparent_color_target; // Point fragment state to the new target
        transparent_pipeline_desc.fragment = &fragment_state;

        transparent_pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        transparent_pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
        transparent_pipeline_desc.primitive.cullMode = WGPUCullMode_None; // No culling for transparent objects

        transparent_pipeline_desc.depthStencil = &depth_stencil_state; // Reuse depth state

        transparent_pipeline_desc.multisample.count = 1;
        transparent_pipeline_desc.multisample.mask = 0xFFFFFFFF;

        transparent_render_pipeline = wgpuDeviceCreateRenderPipeline(device, &transparent_pipeline_desc);
        if (!transparent_render_pipeline)
        {
            throw std::runtime_error("Failed to create transparent render pipeline.");
        }

        // Release the layout, it's not needed anymore
        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuShaderModuleRelease(shader_module); // Release shader module once pipelines are created

        // --- Initialize Game State ---
        glm::vec3 initial_pos(CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f + 2.0f, CHUNK_DEPTH / 2.0f);

        state.emplace(AppState{
            .player = Player(initial_pos, -3.14159f / 2.0f, 0.0f, 0.003f),

            //
        });

        world = World();

        // --- Initialize Camera ---
        camera_uniform = CameraUniform();
        WGPUBufferDescriptor cam_buff_desc = {}; // Zero-initialize!
        cam_buff_desc.label = flint_utils::makeStringView("Camera Buffer");
        cam_buff_desc.size = sizeof(CameraUniform);
        cam_buff_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        camera_buffer = wgpuDeviceCreateBuffer(device, &cam_buff_desc);
        if (!camera_buffer)
        {
            throw std::runtime_error("Failed to create camera buffer.");
        }
        WGPUBindGroupEntry camera_bg_entry = {};
        camera_bg_entry.binding = 0;
        camera_bg_entry.buffer = camera_buffer;
        camera_bg_entry.size = sizeof(CameraUniform);

        WGPUBindGroupDescriptor camera_bg_desc = {};
        camera_bg_desc.label = flint_utils::makeStringView("Camera Bind Group");
        camera_bg_desc.layout = camera_bind_group_layout;
        camera_bg_desc.entryCount = 1;
        camera_bg_desc.entries = &camera_bg_entry;
        camera_bind_group = wgpuDeviceCreateBindGroup(device, &camera_bg_desc);
        if (!camera_bind_group) {
            throw std::runtime_error("Failed to create camera bind group.");
        }

        // --- Initialize Textures ---
        block_atlas_texture.emplace(Texture::create_placeholder(device, queue, flint_utils::makeStringView("Placeholder Block Atlas")));

        WGPUBindGroupEntry texture_bg_entries[2] = {};
        texture_bg_entries[0].binding = 0;
        texture_bg_entries[0].textureView = block_atlas_texture->view; // WGPUTextureView
        texture_bg_entries[1].binding = 1;
        texture_bg_entries[1].sampler = block_atlas_texture->sampler; // WGPUSampler

        WGPUBindGroupDescriptor texture_bg_desc = {};
        texture_bg_desc.label = flint_utils::makeStringView("Block Atlas Bind Group");
        texture_bg_desc.layout = texture_bind_group_layout;
        texture_bg_desc.entryCount = 2;
        texture_bg_desc.entries = texture_bg_entries;
        block_atlas_bind_group = wgpuDeviceCreateBindGroup(device, &texture_bg_desc);
        if (!block_atlas_bind_group) {
            throw std::runtime_error("Failed to create block atlas bind group.");
        }

        // --- Initialize Depth Texture ---
        resize(width, height);

        // --- Initialize UI and Overlays ---
        // TODO: This initialization is complex and depends on BGLs created above.
        // debug_overlay = DebugOverlay(device, &config);
        // crosshair = ui::Crosshair(device, &config);
        // ... and so on for all UI elements.

        set_mouse_grab(true);
    }

    App::~App()
    {
        // Release all Dawn objects in reverse order of creation.
        // TODO: This is a partial list. All created objects must be released.
        wgpuRenderPipelineRelease(render_pipeline);
        wgpuRenderPipelineRelease(transparent_render_pipeline);
        wgpuBufferRelease(camera_buffer);
        wgpuTextureViewRelease(depth_texture_view);
        wgpuTextureRelease(depth_texture);
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuSurfaceRelease(surface);
        wgpuInstanceRelease(instance);
    }

    void App::resize(uint32_t new_width, uint32_t new_height)
    {
        if (new_width > 0 && new_height > 0)
        {
            width = new_width;
            height = new_height;
            config.width = new_width;
            config.height = new_height;
            wgpuSurfaceConfigure(surface, &config);

            // Recreate depth texture
            if (depth_texture_view)
                wgpuTextureViewRelease(depth_texture_view);
            if (depth_texture)
                wgpuTextureRelease(depth_texture);

            WGPUTextureDescriptor depth_desc = {};
            depth_desc.size = {new_width, new_height, 1};
            depth_desc.format = WGPUTextureFormat_Depth32Float; // Common depth format
            depth_desc.usage = WGPUTextureUsage_RenderAttachment;
            depth_texture = wgpuDeviceCreateTexture(device, &depth_desc);
            depth_texture_view = wgpuTextureCreateView(depth_texture, nullptr);

            // TODO when ui is implemented
            // // Resize UI elements
            // debug_overlay.resize(new_width, new_height, queue);
            // crosshair.resize(new_width, new_height, queue);
            // ui_text.resize(new_width, new_height, queue);
        }
    }

    void App::handle_input(const SDL_Event &event)
    {
        // TODO: Translate the complex input logic from main.rs's App::handle_window_event
        // This involves handling keyboard, mouse, and window events, and managing mouse grab state.
    }

    void App::update()
    {
        if (!state.has_value()) return;

        // Determine active chunks and build meshes
        glm::vec3 player_pos = state->player.position;
        int current_chunk_x = static_cast<int>(floor(player_pos.x / CHUNK_WIDTH));
        int current_chunk_z = static_cast<int>(floor(player_pos.z / CHUNK_DEPTH));

        active_chunk_coords.clear();
        int render_distance = 1; // Minimal render distance for now
        for (int dx = -render_distance; dx <= render_distance; ++dx) {
            for (int dz = -render_distance; dz <= render_distance; ++dz) {
                int target_cx = current_chunk_x + dx;
                int target_cz = current_chunk_z + dz;
                active_chunk_coords.push_back({target_cx, target_cz});
                world.get_or_create_chunk(target_cx, target_cz);
            }
        }

        std::vector<std::pair<int, int>> coords_to_mesh;
        for (const auto& coords : active_chunk_coords) {
            if (chunk_render_data.find(coords) == chunk_render_data.end()) {
                coords_to_mesh.push_back(coords);
            }
        }

        // Sorting and deduping is less critical here than in Rust due to map properties, but good practice
        std::sort(coords_to_mesh.begin(), coords_to_mesh.end());
        coords_to_mesh.erase(std::unique(coords_to_mesh.begin(), coords_to_mesh.end()), coords_to_mesh.end());

        for (const auto& coords : coords_to_mesh) {
            build_or_rebuild_chunk_mesh(coords.first, coords.second);
        }

        // Update camera
        glm::vec3 camera_eye = state->player.position + glm::vec3(0.0f, PLAYER_EYE_HEIGHT, 0.0f);
        float yaw = state->player.yaw;
        float pitch = state->player.pitch;

        glm::vec3 camera_front;
        camera_front.x = cos(yaw) * cos(pitch);
        camera_front.y = sin(pitch);
        camera_front.z = sin(yaw) * cos(pitch);
        camera_front = glm::normalize(camera_front);

        glm::mat4 view_matrix = glm::lookAt(camera_eye, camera_eye + camera_front, glm::vec3(0.0f, 1.0f, 0.0f));

        float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
        float fovy_radians = glm::radians(45.0f);
        float znear = 0.1f;
        float zfar = 1000.0f;
        glm::mat4 projection_matrix = glm::perspective(fovy_radians, aspect_ratio, znear, zfar);

        // GLM is column-major, and WebGPU expects column-major, so this is fine.
        camera_uniform.view_proj = projection_matrix * view_matrix;

        wgpuQueueWriteBuffer(queue, camera_buffer, 0, &camera_uniform, sizeof(CameraUniform));
    }

    void App::render()
    {
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);

        if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
            if (surface_texture.texture) wgpuTextureRelease(surface_texture.texture);
            wgpuSurfaceConfigure(surface, &config);
            return;
        }

        // Stop rendering if we didn't get a texture.
        if (!surface_texture.texture) {
            return;
        }

        WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);
        if (!view) {
            wgpuTextureRelease(surface_texture.texture);
            throw std::runtime_error("Failed to create surface texture view.");
        }

        WGPUCommandEncoderDescriptor enc_desc = {};
        enc_desc.label = flint_utils::makeStringView("Render Encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &enc_desc);

        {
            WGPURenderPassColorAttachment color_attachment = {};
            color_attachment.view = view;
            color_attachment.loadOp = WGPULoadOp_Clear;
            color_attachment.storeOp = WGPUStoreOp_Store;
            color_attachment.clearValue = {0.1, 0.2, 0.3, 1.0};

            WGPURenderPassDepthStencilAttachment depth_attachment = {};
            depth_attachment.view = depth_texture_view;
            depth_attachment.depthLoadOp = WGPULoadOp_Clear;
            depth_attachment.depthStoreOp = WGPUStoreOp_Store;
            depth_attachment.depthClearValue = 1.0f;

            WGPURenderPassDescriptor pass_desc = {};
            pass_desc.label = flint_utils::makeStringView("Main Render Pass");
            pass_desc.colorAttachmentCount = 1;
            pass_desc.colorAttachments = &color_attachment;
            pass_desc.depthStencilAttachment = &depth_attachment;

            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);

            // Draw opaque geometry
            wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, camera_bind_group, 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, block_atlas_bind_group, 0, nullptr);

            for (const auto& coords : active_chunk_coords) {
                auto it = chunk_render_data.find(coords);
                if (it != chunk_render_data.end()) {
                    const auto& chunk_data = it->second;
                    if (chunk_data.opaque_buffers && chunk_data.opaque_buffers->num_indices > 0) {
                        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, chunk_data.opaque_buffers->vertex_buffer, 0, WGPU_WHOLE_SIZE);
                        wgpuRenderPassEncoderSetIndexBuffer(render_pass, chunk_data.opaque_buffers->index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
                        wgpuRenderPassEncoderDrawIndexed(render_pass, chunk_data.opaque_buffers->num_indices, 1, 0, 0, 0);
                    }
                }
            }

            // TODO: Draw transparent geometry

            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);
        }

        WGPUCommandBufferDescriptor cmd_buff_desc = {};
        cmd_buff_desc.label = flint_utils::makeStringView("Command Buffer");
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buff_desc);

        wgpuQueueSubmit(queue, 1, &cmd_buffer);
        wgpuSurfacePresent(surface);

        wgpuTextureViewRelease(view);
        wgpuTextureRelease(surface_texture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }

    void App::process_mouse_motion(float delta_x, float delta_y)
    {
        if (mouse_grabbed && !inventory_open && state.has_value())
        {
            state->player.process_mouse_movement(delta_x, delta_y);
        }
    }

    void App::set_mouse_grab(bool grab)
    {
        SDL_SetWindowRelativeMouseMode(window, grab);
        mouse_grabbed = grab;
    }

    const float ATLAS_COLS = 16.0;
    const float ATLAS_ROWS = 1.0;

    void App::build_or_rebuild_chunk_mesh(int chunk_cx, int chunk_cz) {
        std::vector<Vertex> opaque_vertices;
        std::vector<uint16_t> opaque_indices;
        uint16_t opaque_vertex_offset = 0;

        std::vector<Vertex> transparent_vertices;
        std::vector<uint16_t> transparent_indices;
        uint16_t transparent_vertex_offset = 0;

        struct TransparentBlockData {
            Block block;
            int lx, ly, lz;
            glm::vec3 world_center;
        };
        std::vector<TransparentBlockData> transparent_block_render_list;

        const Chunk* chunk = world.get_chunk(chunk_cx, chunk_cz);
        if (!chunk) {
            // This can happen if a chunk goes out of view before its mesh is built.
            chunk_render_data.erase({chunk_cx, chunk_cz});
            return;
        }

        float chunk_world_origin_x = static_cast<float>(chunk_cx * CHUNK_WIDTH);
        float chunk_world_origin_z = static_cast<float>(chunk_cz * CHUNK_DEPTH);

        for (int lx = 0; lx < CHUNK_WIDTH; ++lx) {
            for (int ly = 0; ly < CHUNK_HEIGHT; ++ly) {
                for (int lz = 0; lz < CHUNK_DEPTH; ++lz) {
                    const Block* block = chunk->get_block(lx, ly, lz);
                    if (!block || block->block_type == BlockType::Air) {
                        continue;
                    }

                    bool is_current_block_transparent = block->is_transparent();
                    glm::vec3 current_block_world_center(
                        chunk_world_origin_x + lx + 0.5f,
                        ly + 0.5f,
                        chunk_world_origin_z + lz + 0.5f
                    );

                    const std::array<std::pair<CubeFace, glm::ivec3>, 6> face_definitions = {{
                        {CubeFace::Front, {0, 0, -1}},
                        {CubeFace::Back, {0, 0, 1}},
                        {CubeFace::Right, {1, 0, 0}},
                        {CubeFace::Left, {-1, 0, 0}},
                        {CubeFace::Top, {0, 1, 0}},
                        {CubeFace::Bottom, {0, -1, 0}}
                    }};

                    for (const auto& face_def : face_definitions) {
                        CubeFace face_type = face_def.first;
                        glm::ivec3 offset = face_def.second;

                        float neighbor_world_bx = chunk_world_origin_x + lx + offset.x;
                        float neighbor_world_by = static_cast<float>(ly) + offset.y;
                        float neighbor_world_bz = chunk_world_origin_z + lz + offset.z;

                        bool face_is_visible = false;
                        uint8_t face_sky_light = 0;

                        if (neighbor_world_by >= 0 && neighbor_world_by < CHUNK_HEIGHT) {
                            const Block* neighbor_block = world.get_block_at_world(neighbor_world_bx, neighbor_world_by, neighbor_world_bz);
                            if (!neighbor_block || neighbor_block->is_transparent()) {
                                face_is_visible = true;
                                if (neighbor_block) {
                                    face_sky_light = neighbor_block->sky_light;
                                }
                            }
                        } else {
                            face_is_visible = true; // Face is at the world border (top/bottom)
                        }


                        if (face_is_visible) {
                             if (!is_current_block_transparent) {
                                auto vertices_template = CubeGeometry::get_vertices_template(face_type);
                                const auto& local_indices = CubeGeometry::get_local_indices();

                                float tex_size_x = 1.0f / ATLAS_COLS;
                                float tex_size_y = 1.0f / ATLAS_ROWS;
                                auto all_face_atlas_indices = block->get_texture_atlas_indices();

                                glm::vec3 current_vertex_color; // Not set from template anymore

                                const auto& face_specific_atlas_indices = all_face_atlas_indices[static_cast<int>(face_type)];

                                if (block->block_type == BlockType::Grass) {
                                     if (face_type == CubeFace::Top) {
                                        current_vertex_color = {0.1f, 0.9f, 0.1f};
                                    } else if (face_type == CubeFace::Bottom) {
                                        current_vertex_color = {0.5f, 0.25f, 0.05f};
                                    } else {
                                        current_vertex_color = {0.0f, 0.8f, 0.1f};
                                    }
                                } else {
                                    // A default color for other blocks
                                    current_vertex_color = {0.5f, 0.5f, 0.5f};
                                }


                                float u_min = face_specific_atlas_indices[0] * tex_size_x;
                                float v_min = face_specific_atlas_indices[1] * tex_size_y;
                                float u_max = u_min + tex_size_x;
                                float v_max = v_min + tex_size_y;

                                std::array<glm::vec2, 4> uvs;
                                switch (face_type) {
                                    case CubeFace::Back:
                                    case CubeFace::Top:
                                        uvs = {{{u_min, v_max}, {u_max, v_max}, {u_max, v_min}, {u_min, v_min}}}; // BL, BR, TR, TL
                                        break;
                                    default:
                                        uvs = {{{u_min, v_max}, {u_min, v_min}, {u_max, v_min}, {u_max, v_max}}}; // BL, TL, TR, BR
                                        break;
                                }

                                for (size_t i = 0; i < vertices_template.size(); ++i) {
                                    Vertex v = vertices_template[i];
                                    v.position += current_block_world_center;
                                    v.color = current_vertex_color;
                                    v.uv = uvs[i];
                                    v.sky_light = face_sky_light;
                                    opaque_vertices.push_back(v);
                                }

                                for (uint16_t local_idx : local_indices) {
                                    opaque_indices.push_back(opaque_vertex_offset + local_idx);
                                }
                                opaque_vertex_offset += static_cast<uint16_t>(vertices_template.size());
                            }
                        }
                    }

                    if (is_current_block_transparent) {
                        transparent_block_render_list.push_back({*block, lx, ly, lz, current_block_world_center});
                    }
                }
            }
        }

        // TODO: Implement transparent block meshing logic similar to the opaque one.
        // For now, we'll just create the buffers for the opaque mesh.

        ChunkRenderData& render_data = chunk_render_data[{chunk_cx, chunk_cz}];

        if (!opaque_vertices.empty() && !opaque_indices.empty()) {
            WGPUBufferDescriptor vertex_buff_desc = {};
            vertex_buff_desc.label = flint_utils::makeStringView("Opaque Vertex Buffer");
            vertex_buff_desc.size = opaque_vertices.size() * sizeof(Vertex);
            vertex_buff_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            WGPUBuffer vertex_buffer = wgpuDeviceCreateBuffer(device, &vertex_buff_desc);
            wgpuQueueWriteBuffer(queue, vertex_buffer, 0, opaque_vertices.data(), vertex_buff_desc.size);

            WGPUBufferDescriptor index_buff_desc = {};
            index_buff_desc.label = flint_utils::makeStringView("Opaque Index Buffer");
            index_buff_desc.size = opaque_indices.size() * sizeof(uint16_t);
            index_buff_desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            WGPUBuffer index_buffer = wgpuDeviceCreateBuffer(device, &index_buff_desc);
            wgpuQueueWriteBuffer(queue, index_buffer, 0, opaque_indices.data(), index_buff_desc.size);

            render_data.opaque_buffers.emplace();
            render_data.opaque_buffers->vertex_buffer = vertex_buffer;
            render_data.opaque_buffers->index_buffer = index_buffer;
            render_data.opaque_buffers->num_indices = static_cast<uint32_t>(opaque_indices.size());
        } else {
            render_data.opaque_buffers.reset();
        }

        // In a complete implementation, you would do the same for transparent buffers.
        render_data.transparent_buffers.reset();

        if (!render_data.opaque_buffers && !render_data.transparent_buffers) {
            chunk_render_data.erase({chunk_cx, chunk_cz});
        }
    }

} // namespace flint
