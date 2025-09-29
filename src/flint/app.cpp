#include "app.h"
#include <iostream>
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/utils.h"
#include "init/shader.h"
#include "init/buffer.h"
#include "init/pipeline.h"
#include "shader.wgsl.h"
#include "shader_debug.wgsl.h"
#include <glm/gtc/matrix_transform.hpp>
#include "physics.h"

namespace flint
{
    App::App()
        : m_player(
              // Initial position: center of the chunk, 5 blocks above the surface
              {CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f + 5.0f, CHUNK_DEPTH / 2.0f},
              // Initial orientation: looking forward along -Z
              -90.0f, // yaw
              0.0f,   // pitch
              // Mouse sensitivity
              0.1f)
    {
        std::cout << "Initializing app..." << std::endl;
        m_windowWidth = 800;
        m_windowHeight = 600;
        m_window = init::sdl(m_windowWidth, m_windowHeight);
        SDL_SetWindowRelativeMouseMode(m_window, true);

        init::wgpu(
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

        std::cout << "Creating shaders..." << std::endl;

        m_vertexShader = init::create_shader_module(m_device, "Vertex Shader", WGSL_vertexShaderSource);
        m_fragmentShader = init::create_shader_module(m_device, "Fragment Shader", WGSL_fragmentShaderSource);

        std::cout << "Shaders created successfully" << std::endl;

        std::cout << "Setting up 3D components..." << std::endl;

        // The camera is now controlled by the player, so we initialize it with placeholder values.
        // It will be updated every frame in the `render` loop based on the player's state.
        m_camera = Camera(
            {0.0f, 0.0f, 0.0f},                           // eye (will be overwritten)
            {0.0f, 0.0f, -1.0f},                          // target (will be overwritten)
            {0.0f, 1.0f, 0.0f},                           // up
            (float)m_windowWidth / (float)m_windowHeight, // aspect
            45.0f,                                        // fovy
            0.1f,                                         // znear
            1000.0f                                       // zfar
        );

        // Generate terrain and mesh
        std::cout << "Generating terrain..." << std::endl;
        m_chunk.generateTerrain();
        std::cout << "Generating chunk mesh..." << std::endl;
        m_chunkMesh.generate(m_device, m_chunk);
        std::cout << "Chunk mesh generated." << std::endl;

        // Create uniform buffer for camera matrices
        m_uniformBuffer = init::create_uniform_buffer(m_device, "Camera Uniform Buffer", sizeof(CameraUniform));

        std::cout << "3D components ready!" << std::endl;

        // Create depth texture
        WGPUTextureDescriptor depthTextureDesc = {};
        depthTextureDesc.dimension = WGPUTextureDimension_2D;
        depthTextureDesc.format = m_depthTextureFormat;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = 1;
        depthTextureDesc.size = {static_cast<uint32_t>(m_windowWidth), static_cast<uint32_t>(m_windowHeight), 1};
        depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
        depthTextureDesc.viewFormatCount = 1;
        depthTextureDesc.viewFormats = &m_depthTextureFormat;
        m_depthTexture = wgpuDeviceCreateTexture(m_device, &depthTextureDesc);

        WGPUTextureViewDescriptor depthTextureViewDesc = {};
        depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        depthTextureViewDesc.baseArrayLayer = 0;
        depthTextureViewDesc.arrayLayerCount = 1;
        depthTextureViewDesc.baseMipLevel = 0;
        depthTextureViewDesc.mipLevelCount = 1;
        depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
        depthTextureViewDesc.format = m_depthTextureFormat;
        m_depthTextureView = wgpuTextureCreateView(m_depthTexture, &depthTextureViewDesc);

        m_renderPipeline = init::create_render_pipeline(
            m_device,
            m_vertexShader,
            m_fragmentShader,
            m_surfaceFormat,
            m_depthTextureFormat,
            &m_bindGroupLayout);

        m_bindGroup = init::create_bind_group(m_device, m_bindGroupLayout, m_uniformBuffer);

        // =================================================================
        // Setup Debug Bounding Box Renderer
        // =================================================================
        std::cout << "Setting up debug renderer..." << std::endl;
        m_debugMesh.generate(m_device);

        WGPUShaderModule debugShaderModule = init::create_shader_module(m_device, "Debug Shader", WGSL_debugShaderSource);

        // --- Debug Bind Group Layout (for model matrix and color) ---
        WGPUBindGroupLayoutEntry debugBglEntries[2] = {};
        // Model Uniform
        debugBglEntries[0].binding = 0;
        debugBglEntries[0].visibility = WGPUShaderStage_Vertex;
        debugBglEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
        // Color Uniform
        debugBglEntries[1].binding = 1;
        debugBglEntries[1].visibility = WGPUShaderStage_Fragment;
        debugBglEntries[1].buffer.type = WGPUBufferBindingType_Uniform;

        WGPUBindGroupLayoutDescriptor debugBglDesc = {};
        debugBglDesc.entryCount = 2;
        debugBglDesc.entries = debugBglEntries;
        m_debugBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &debugBglDesc);


        // --- Debug Pipeline Layout ---
        // The debug pipeline uses two bind groups:
        // Group 0: Camera (re-uses the layout from the main pipeline)
        // Group 1: Model/Color (the new layout we just created)
        WGPUBindGroupLayout DbgPipeline_bgls[2] = {m_bindGroupLayout, m_debugBindGroupLayout};
        WGPUPipelineLayoutDescriptor debugPipelineLayoutDesc = {};
        debugPipelineLayoutDesc.bindGroupLayoutCount = 2;
        debugPipelineLayoutDesc.bindGroupLayouts = DbgPipeline_bgls;
        WGPUPipelineLayout debugPipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &debugPipelineLayoutDesc);

        // --- Debug Render Pipeline ---
        // The following block uses C99-style designated initializers for clarity and
        // to correctly initialize all nested WebGPU descriptor structs.

        // Vertex buffer layout
        WGPUVertexAttribute debugVertexAttrib = {
            .format = WGPUVertexFormat_Float32x3, // vec3 position
            .offset = 0,
            .shaderLocation = 0,
        };
        WGPUVertexBufferLayout debugVertexBufferLayout = {
            .arrayStride = 3 * sizeof(float),
            .attributeCount = 1,
            .attributes = &debugVertexAttrib,
        };

        // Blend state for transparency
        WGPUBlendState blendState = {
            .color = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha, .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha},
            .alpha = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One, .dstFactor = WGPUBlendFactor_Zero},
        };

        // Color target state
        WGPUColorTargetState debugColorTarget = {
            .format = m_surfaceFormat,
            .blend = &blendState,
            .writeMask = WGPUColorWriteMask_All,
        };

        // Fragment state
        WGPUFragmentState debugFragmentState = {
            .module = debugShaderModule,
            .entryPoint = "fs_main",
            .targetCount = 1,
            .targets = &debugColorTarget,
        };

        WGPURenderPipelineDescriptor debugPipelineDesc = {
            .layout = debugPipelineLayout,
            .vertex = {
                .module = debugShaderModule,
                .entryPoint = "vs_main",
                .bufferCount = 1,
                .buffers = &debugVertexBufferLayout,
            },
            .primitive = {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .cullMode = WGPUCullMode_None,
            },
            .depthStencil =
                &(WGPUDepthStencilState){
                    .format = m_depthTextureFormat,
                    .depthWriteEnabled = WGPU_TRUE,
                    .depthCompare = WGPUCompareFunction_Less,
                },
            .multisample = {.count = 1},
            .fragment = &debugFragmentState,
        };

        m_debugRenderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &debugPipelineDesc);
        std::cout << "Debug pipeline created." << std::endl;

        // --- Create Uniform Buffers & Bind Group for Debug Renderer ---
        m_debugModelUniformBuffer = init::create_uniform_buffer(m_device, "Debug Model Uniform Buffer", sizeof(glm::mat4));
        m_debugColorUniformBuffer = init::create_uniform_buffer(m_device, "Debug Color Uniform Buffer", sizeof(glm::vec4));

        WGPUBindGroupEntry debugBgEntries[2] = {};
        // Model Uniform
        debugBgEntries[0].binding = 0;
        debugBgEntries[0].buffer = m_debugModelUniformBuffer;
        debugBgEntries[0].size = sizeof(glm::mat4);
        // Color Uniform
        debugBgEntries[1].binding = 1;
        debugBgEntries[1].buffer = m_debugColorUniformBuffer;
        debugBgEntries[1].size = sizeof(glm::vec4);

        WGPUBindGroupDescriptor debugBgDesc = {};
        debugBgDesc.layout = m_debugBindGroupLayout;
        debugBgDesc.entryCount = 2;
        debugBgDesc.entries = debugBgEntries;
        m_debugBindGroup = wgpuDeviceCreateBindGroup(m_device, &debugBgDesc);

        // Cleanup temporary objects
        wgpuPipelineLayoutRelease(debugPipelineLayout);
        wgpuShaderModuleRelease(debugShaderModule);

        // ====
        m_running = true;
    }

    void App::run()
    {
        std::cout << "Running app..." << std::endl;

        uint64_t last_tick = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (m_running)
        {
            // Calculate delta time
            uint64_t current_tick = SDL_GetPerformanceCounter();
            float dt = static_cast<float>(current_tick - last_tick) / static_cast<float>(SDL_GetPerformanceFrequency());
            last_tick = current_tick;

            // Handle events
            while (SDL_PollEvent(&e))
            {
                // Let the player handle keyboard input
                m_player.handle_input(e);

                if (e.type == SDL_EVENT_QUIT)
                {
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_MOUSE_MOTION)
                {
                    m_player.process_mouse_movement(static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel));
                }
            }

            // Update player physics and state
            m_player.update(dt, m_chunk);

            // Render the scene
            render();
        }
    }

    void App::render()
    {
        // Update camera from player state
        glm::vec3 player_pos = m_player.get_position();
        float player_yaw_deg = m_player.get_yaw();
        float player_pitch_deg = m_player.get_pitch();

        m_camera.eye = player_pos + glm::vec3(0.0f, physics::PLAYER_EYE_HEIGHT, 0.0f);

        float yaw_rad = glm::radians(player_yaw_deg);
        float pitch_rad = glm::radians(player_pitch_deg);

        glm::vec3 front;
        front.x = cos(yaw_rad) * cos(pitch_rad);
        front.y = sin(pitch_rad);
        front.z = sin(yaw_rad) * cos(pitch_rad);
        m_camera.target = m_camera.eye + glm::normalize(front);

        // Update the uniform buffer with the new camera view-projection matrix
        m_cameraUniform.updateViewProj(m_camera);
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

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

            WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
            depthStencilAttachment.view = m_depthTextureView;
            depthStencilAttachment.depthClearValue = 1.0f;
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
            depthStencilAttachment.depthReadOnly = false;
            depthStencilAttachment.stencilClearValue = 0;
            depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
            depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
            depthStencilAttachment.stencilReadOnly = true;

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.nextInChain = nullptr;
            renderPassDesc.label = {nullptr, 0};
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

            // NEW: 3D Cube rendering instead of triangle
            wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);

            // Bind the uniform buffer (camera matrix)
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);

            // Draw the chunk
            m_chunkMesh.render(renderPass);

            // --- Draw Player Bounding Box ---
            {
                // 1. Get player's world AABB and calculate its properties
                physics::AABB player_box = m_player.get_world_bounding_box();
                glm::vec3 box_size = player_box.max - player_box.min;
                glm::vec3 box_center = player_box.min + (box_size * 0.5f);

                // 2. Create model matrix to scale and translate the unit debug cube
                glm::mat4 model_matrix = glm::mat4(1.0f);
                model_matrix = glm::translate(model_matrix, box_center);
                model_matrix = glm::scale(model_matrix, box_size);

                // 3. Define color (hot pink, semi-transparent)
                glm::vec4 pink_color(1.0f, 0.41f, 0.7f, 0.5f);

                // 4. Update uniform buffers
                wgpuQueueWriteBuffer(m_queue, m_debugModelUniformBuffer, 0, &model_matrix, sizeof(glm::mat4));
                wgpuQueueWriteBuffer(m_queue, m_debugColorUniformBuffer, 0, &pink_color, sizeof(glm::vec4));

                // 5. Set pipeline and bind groups
                wgpuRenderPassEncoderSetPipeline(renderPass, m_debugRenderPipeline);
                wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);     // Group 0 for camera
                wgpuRenderPassEncoderSetBindGroup(renderPass, 1, m_debugBindGroup, 0, nullptr); // Group 1 for model/color

                // 6. Draw the debug mesh
                m_debugMesh.render(renderPass);
            }

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

        m_chunkMesh.cleanup();
        m_debugMesh.cleanup();

        // Release debug resources
        if (m_debugBindGroup) wgpuBindGroupRelease(m_debugBindGroup);
        if (m_debugBindGroupLayout) wgpuBindGroupLayoutRelease(m_debugBindGroupLayout);
        if (m_debugModelUniformBuffer) wgpuBufferRelease(m_debugModelUniformBuffer);
        if (m_debugColorUniformBuffer) wgpuBufferRelease(m_debugColorUniformBuffer);
        if (m_debugRenderPipeline) wgpuRenderPipelineRelease(m_debugRenderPipeline);

        if (m_depthTextureView)
            wgpuTextureViewRelease(m_depthTextureView);
        if (m_depthTexture)
            wgpuTextureRelease(m_depthTexture);

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
