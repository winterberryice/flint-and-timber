#include "app.h"
#include <iostream>
#include <vector>
#include <string>
#include "glm/gtc/matrix_transform.hpp"
// ifstream include
#include <fstream>

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

        //         std::cout << "Creating triangle vertex data..." << std::endl;

        //         // Triangle vertices in NDC coordinates (x, y, z)
        //         float triangleVertices[] = {
        //             0.0f, 0.5f, 0.0f,   // Top vertex
        //             -0.5f, -0.5f, 0.0f, // Bottom left
        //             0.5f, -0.5f, 0.0f   // Bottom right
        //         };

        //         // Create vertex buffer descriptor
        //         WGPUBufferDescriptor vertexBufferDesc = {};
        //         vertexBufferDesc.nextInChain = nullptr;
        //         vertexBufferDesc.label = makeStringView("Triangle Vertex Buffer");
        //         vertexBufferDesc.size = sizeof(triangleVertices);
        //         vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        //         vertexBufferDesc.mappedAtCreation = false;

        //         // Create the vertex buffer
        //         m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
        //         if (!m_vertexBuffer)
        //         {
        //             std::cerr << "Failed to create vertex buffer!" << std::endl;
        //             return -1;
        //         }

        //         // Upload vertex data to GPU
        //         wgpuQueueWriteBuffer(m_queue, m_vertexBuffer, 0, triangleVertices, sizeof(triangleVertices));

        //         std::cout << "Vertex buffer created and data uploaded" << std::endl;

        //         std::cout << "Creating shaders..." << std::endl;

        //         const char *vertexShaderSource = R"(
        // struct Uniforms {
        //     viewProjectionMatrix: mat4x4<f32>,
        // };

        // @group(0) @binding(0) var<uniform> uniforms: Uniforms;

        // struct VertexInput {
        //     @location(0) position: vec3<f32>,
        //     @location(1) color: vec3<f32>,
        // };

        // struct VertexOutput {
        //     @builtin(position) position: vec4<f32>,
        //     @location(0) color: vec3<f32>,
        // };

        // @vertex
        // fn vs_main(input: VertexInput) -> VertexOutput {
        //     var output: VertexOutput;
        //     output.position = uniforms.viewProjectionMatrix * vec4<f32>(input.position, 1.0);
        //     output.color = input.color;
        //     return output;
        // }
        // )";

        //         const char *fragmentShaderSource = R"(
        // @fragment
        // fn fs_main(@location(0) color: vec3<f32>) -> @location(0) vec4<f32> {
        //     return vec4<f32>(color, 1.0);
        // }
        // )";

        //         // Create vertex shader module
        //         WGPUShaderModuleWGSLDescriptor vertexShaderWGSLDesc = {};
        //         vertexShaderWGSLDesc.chain.next = nullptr;
        //         vertexShaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        //         vertexShaderWGSLDesc.code = makeStringView(vertexShaderSource);

        //         WGPUShaderModuleDescriptor vertexShaderDesc = {};
        //         vertexShaderDesc.nextInChain = &vertexShaderWGSLDesc.chain;
        //         vertexShaderDesc.label = makeStringView("Vertex Shader");

        //         m_vertexShader = wgpuDeviceCreateShaderModule(m_device, &vertexShaderDesc);

        //         // Create fragment shader module
        //         WGPUShaderModuleWGSLDescriptor fragmentShaderWGSLDesc = {};
        //         fragmentShaderWGSLDesc.chain.next = nullptr;
        //         fragmentShaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        //         fragmentShaderWGSLDesc.code = makeStringView(fragmentShaderSource);

        //         WGPUShaderModuleDescriptor fragmentShaderDesc = {};
        //         fragmentShaderDesc.nextInChain = &fragmentShaderWGSLDesc.chain;
        //         fragmentShaderDesc.label = makeStringView("Fragment Shader");

        //         m_fragmentShader = wgpuDeviceCreateShaderModule(m_device, &fragmentShaderDesc);

        //         if (!m_vertexShader || !m_fragmentShader)
        //         {
        //             std::cerr << "Failed to create shaders!" << std::endl;
        //             return false; // Assuming your init function returns bool
        //         }

        //         std::cout << "Shaders created successfully" << std::endl;

        //         std::cout << "Setting up 3D components..." << std::endl;

        //         // Initialize chunk
        //         if (!m_chunk.initialize(m_device))
        //         {
        //             std::cerr << "Failed to initialize chunk!" << std::endl;
        //             return false;
        //         }

        //         // Create uniform buffer for camera matrices
        //         WGPUBufferDescriptor uniformBufferDesc = {};
        //         uniformBufferDesc.size = sizeof(glm::mat4); // Size of view-projection matrix
        //         uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        //         uniformBufferDesc.mappedAtCreation = false;

        //         m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);
        //         if (!m_uniformBuffer)
        //         {
        //             std::cerr << "Failed to create uniform buffer!" << std::endl;
        //             return false;
        //         }

        //         std::cout << "3D components ready!" << std::endl;

        //         std::cout << "Creating render pipeline..." << std::endl;

        //         // Create bind group layout for uniforms first
        //         WGPUBindGroupLayoutEntry bindingLayout = {};
        //         bindingLayout.binding = 0;
        //         bindingLayout.visibility = WGPUShaderStage_Vertex;
        //         bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        //         bindingLayout.buffer.minBindingSize = sizeof(glm::mat4);

        //         WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        //         bindGroupLayoutDesc.entryCount = 1;
        //         bindGroupLayoutDesc.entries = &bindingLayout;

        //         m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDesc);
        //         if (!m_bindGroupLayout)
        //         {
        //             std::cerr << "Failed to create bind group layout!" << std::endl;
        //             return false;
        //         }

        //         // Create pipeline layout using our bind group layout
        //         WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        //         pipelineLayoutDesc.bindGroupLayoutCount = 1;
        //         pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;

        //         WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);

        //         // NEW: Define vertex buffer layout for position + color
        //         WGPUVertexAttribute vertexAttributes[2] = {};

        //         // Position attribute (location 0)
        //         vertexAttributes[0].format = WGPUVertexFormat_Float32x3; // vec3f position
        //         vertexAttributes[0].offset = 0;
        //         vertexAttributes[0].shaderLocation = 0;

        //         // Color attribute (location 1)
        //         vertexAttributes[1].format = WGPUVertexFormat_Float32x3; // vec3f color
        //         vertexAttributes[1].offset = 3 * sizeof(float);          // After position
        //         vertexAttributes[1].shaderLocation = 1;

        //         WGPUVertexBufferLayout vertexBufferLayout = {};
        //         vertexBufferLayout.arrayStride = 6 * sizeof(float); // 3 floats position + 3 floats color
        //         vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        //         vertexBufferLayout.attributeCount = 2; // position + color
        //         vertexBufferLayout.attributes = vertexAttributes;

        //         // Create render pipeline descriptor
        //         WGPURenderPipelineDescriptor pipelineDescriptor = {};
        //         pipelineDescriptor.label = makeStringView("Cube Render Pipeline");

        //         // Vertex state
        //         pipelineDescriptor.vertex.module = m_vertexShader;
        //         pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        //         pipelineDescriptor.vertex.bufferCount = 1;
        //         pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        //         // Fragment state
        //         WGPUFragmentState fragmentState = {};
        //         fragmentState.module = m_fragmentShader;
        //         fragmentState.entryPoint = makeStringView("fs_main");

        //         // Color target (what we're rendering to)
        //         WGPUColorTargetState colorTarget = {};
        //         colorTarget.format = m_surfaceFormat;
        //         colorTarget.writeMask = WGPUColorWriteMask_All;

        //         fragmentState.targetCount = 1;
        //         fragmentState.targets = &colorTarget;
        //         pipelineDescriptor.fragment = &fragmentState;

        //         // Primitive state - IMPORTANT: Enable depth testing for 3D
        //         pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        //         pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        //         pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        //         pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back; // Enable back-face culling

        //         // Multisample state
        //         pipelineDescriptor.multisample.count = 1;
        //         pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        //         pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        //         // Use our custom layout (not auto)
        //         pipelineDescriptor.layout = pipelineLayout;

        //         // Create the pipeline
        //         m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDescriptor);

        //         // Clean up temporary layout
        //         wgpuPipelineLayoutRelease(pipelineLayout);

        //         if (!m_renderPipeline)
        //         {
        //             std::cerr << "Failed to create render pipeline!" << std::endl;
        //             return false;
        //         }

        //         // Create bind group for uniforms
        //         WGPUBindGroupEntry binding = {};
        //         binding.binding = 0;
        //         binding.buffer = m_uniformBuffer;
        //         binding.offset = 0;
        //         binding.size = sizeof(glm::mat4);

        //         WGPUBindGroupDescriptor bindGroupDesc = {};
        //         bindGroupDesc.layout = m_bindGroupLayout;
        //         bindGroupDesc.entryCount = 1;
        //         bindGroupDesc.entries = &binding;

        //         m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &bindGroupDesc);

        //         std::cout << "Render pipeline created successfully" << std::endl;

        //         // ====
        //         m_running = true;

        // end new ================================================

        // surface = SDL_GetWGPUSurface(instance, window);

        // WGPURequestAdapterOptions adapter_options = {};
        // adapter_options.compatibleSurface = surface;
        // adapter = wgpuInstanceRequestAdapter(instance, &adapter_options);

        // WGPUDeviceDescriptor device_desc = {};
        // device_desc.errorCallback = PrintDeviceError;
        // device = wgpuAdapterRequestDevice(adapter, &device_desc);
        // queue = wgpuDeviceGetQueue(device);

        // WGPUSurfaceCapabilities caps;
        // wgpuSurfaceGetCapabilities(surface, adapter, &caps);
        // WGPUTextureFormat surface_format = caps.formats[0];

        // config.usage = WGPUTextureUsage_RenderAttachment;
        // config.format = surface_format;
        // config.width = width;
        // config.height = height;
        // config.presentMode = WGPUPresentMode_Fifo;
        // config.alphaMode = caps.alphaModes[0];
        // wgpuSurfaceConfigure(surface, &config);

        // --- Main Render Pipeline ---
        // TODO: The shader loading logic should be centralized.
        // This is a placeholder for reading the shader file.
        // TODO path with src or not?
        std::string shader_code = readFile("src/shader.wgsl"); //  "/* shader code would be loaded here */";

        WGPUShaderModuleWGSLDescriptor shader_wgsl = {};
        shader_wgsl.chain.next = nullptr;
        shader_wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
        shader_wgsl.code = makeStringView(shader_code.c_str());
        WGPUShaderModuleDescriptor shader_desc = {.nextInChain = &shader_wgsl.chain};
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

        // Create BGLs
        // Camera BGL
        WGPUBindGroupLayoutEntry camera_bgl_entry = {};
        camera_bgl_entry.binding = 0;
        camera_bgl_entry.visibility = WGPUShaderStage_Vertex;
        camera_bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;
        camera_bgl_entry.buffer.hasDynamicOffset = false;
        camera_bgl_entry.buffer.minBindingSize = sizeof(CameraUniform);

        WGPUBindGroupLayoutDescriptor camera_bgl_desc = {};
        camera_bgl_desc.label = "camera_bind_group_layout";
        camera_bgl_desc.entryCount = 1;
        camera_bgl_desc.entries = &camera_bgl_entry;
        camera_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &camera_bgl_desc);

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
        texture_bgl_desc.label = "texture_bind_group_layout";
        texture_bgl_desc.entryCount = 2;
        texture_bgl_desc.entries = texture_bgl_entries;
        texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &texture_bgl_desc);

        std::vector<WGPUBindGroupLayout> bgls = {camera_bind_group_layout, texture_bind_group_layout};
        WGPUPipelineLayoutDescriptor layout_desc = {.bindGroupLayoutCount = (uint32_t)bgls.size(), .bindGroupLayouts = bgls.data()};
        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

        // --- Create Render Pipelines ---

        // Shared Vertex State
        WGPUVertexState vertex_state = {};
        vertex_state.module = shader_module;
        vertex_state.entryPoint = "vs_main";
        WGPUVertexBufferLayout vertex_buffer_layout = Vertex::get_layout();
        vertex_state.bufferCount = 1;
        vertex_state.buffers = &vertex_buffer_layout;

        // Shared Depth/Stencil State
        WGPUDepthStencilState depth_stencil_state = {};
        depth_stencil_state.format = WGPUTextureFormat_Depth32Float;
        depth_stencil_state.depthWriteEnabled = true;
        depth_stencil_state.depthCompare = WGPUCompareFunction_Less;

        // --- Opaque Render Pipeline ---
        WGPURenderPipelineDescriptor opaque_pipeline_desc = {};
        opaque_pipeline_desc.label = "Opaque Render Pipeline";
        opaque_pipeline_desc.layout = pipeline_layout;
        opaque_pipeline_desc.vertex = vertex_state;

        WGPUBlendState opaque_blend_state = {};
        opaque_blend_state.color = {WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha};
        opaque_blend_state.alpha = {WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};

        WGPUColorTargetState opaque_color_target = {};
        opaque_color_target.format = config.format;
        opaque_color_target.blend = &opaque_blend_state;
        opaque_color_target.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState opaque_fragment_state = {};
        opaque_fragment_state.module = shader_module;
        opaque_fragment_state.entryPoint = "fs_main";
        opaque_fragment_state.targetCount = 1;
        opaque_fragment_state.targets = &opaque_color_target;
        opaque_pipeline_desc.fragment = &opaque_fragment_state;

        opaque_pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        opaque_pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
        opaque_pipeline_desc.primitive.cullMode = WGPUCullMode_Back;

        opaque_pipeline_desc.depthStencil = &depth_stencil_state;

        opaque_pipeline_desc.multisample.count = 1;
        opaque_pipeline_desc.multisample.mask = 0xFFFFFFFF;

        render_pipeline = wgpuDeviceCreateRenderPipeline(device, &opaque_pipeline_desc);

        // --- Transparent Render Pipeline ---
        WGPURenderPipelineDescriptor transparent_pipeline_desc = opaque_pipeline_desc; // Start with a copy
        transparent_pipeline_desc.label = "Transparent Render Pipeline";

        WGPUColorTargetState transparent_color_target = {};
        transparent_color_target.format = config.format;
        transparent_color_target.blend = nullptr; // No blending for transparent pass in this setup
        transparent_color_target.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState transparent_fragment_state = {};
        transparent_fragment_state.module = shader_module;
        transparent_fragment_state.entryPoint = "fs_main";
        transparent_fragment_state.targetCount = 1;
        transparent_fragment_state.targets = &transparent_color_target;
        transparent_pipeline_desc.fragment = &transparent_fragment_state;

        transparent_pipeline_desc.primitive.cullMode = WGPUCullMode_None; // No culling

        transparent_render_pipeline = wgpuDeviceCreateRenderPipeline(device, &transparent_pipeline_desc);

        // Release the layout, it's not needed anymore
        wgpuPipelineLayoutRelease(pipeline_layout);

        // --- Initialize Game State ---
        glm::vec3 initial_pos(CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f + 2.0f, CHUNK_DEPTH / 2.0f);
        player = Player(initial_pos, -3.14159f / 2.0f, 0.0f, 0.003f);

        world = World();

        // --- Initialize Camera ---
        camera_uniform = CameraUniform();
        WGPUBufferDescriptor cam_buff_desc = {.size = sizeof(CameraUniform), .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst};
        camera_buffer = wgpuDeviceCreateBuffer(device, &cam_buff_desc);
        // ... create camera_bind_group

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

            // Resize UI elements
            debug_overlay.resize(new_width, new_height, queue);
            crosshair.resize(new_width, new_height, queue);
            ui_text.resize(new_width, new_height, queue);
        }
    }

    void App::handle_input(const SDL_Event &event)
    {
        // TODO: Translate the complex input logic from main.rs's App::handle_window_event
        // This involves handling keyboard, mouse, and window events, and managing mouse grab state.
    }

    void App::update()
    {
        // TODO: Translate the update logic from State::update()
        // This is a very large function that involves:
        // 1. Handling inventory/block interactions based on input.
        // 2. Updating active chunks and building new meshes.
        // 3. Running player physics and raycasting.
        // 4. Updating the camera matrix.
        // 5. Updating the debug overlay.
    }

    void App::render()
    {
        // TODO: Translate the render logic from State::render()
        // This is a very large function that involves multiple render passes:
        // 1. Main pass for opaque world geometry.
        // 2. Second pass for transparent world geometry (sorted back to front).
        // 3. UI pass for hotbar, inventory, crosshair, and rendered items.
        // 4. Text pass for item counts.
        // 5. Debug text pass.

        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
        if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            return;
        }
        WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);

        WGPUCommandEncoderDescriptor enc_desc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &enc_desc);

        // ... create render passes and draw ...

        WGPUCommandBufferDescriptor cmd_buff_desc = {};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buff_desc);
        wgpuQueueSubmit(queue, 1, &cmd_buffer);

        wgpuSurfacePresent(surface);

        wgpuTextureViewRelease(view);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }

    void App::process_mouse_motion(float delta_x, float delta_y)
    {
        if (mouse_grabbed && !inventory_open)
        {
            player.process_mouse_movement(delta_x, delta_y);
        }
    }

    void App::set_mouse_grab(bool grab)
    {
        if (grab)
        {
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        else
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        mouse_grabbed = grab;
    }

    // ... Implementations for build_or_rebuild_chunk_mesh, handle_inventory_interaction, etc.
    // These would be very large and are omitted for brevity, but would follow the logic in `main.rs`.

} // namespace flint
