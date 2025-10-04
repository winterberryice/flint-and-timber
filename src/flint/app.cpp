#include "app.h"
#include <iostream>
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/utils.h"
#include "init/shader.h"
#include "init/buffer.h"
#include "init/pipeline.h"
#include "shader.wgsl.h"

#include "atlas_bytes.hpp"

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

        if (!m_atlas.loadFromMemory(m_device, m_queue, assets_textures_block_atlas_png, assets_textures_block_atlas_png_len))
        {
            std::cerr << "Failed to load texture atlas!" << std::endl;
            // For now, we'll just print an error. In a real app, you might want to exit.
        }

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

        m_bindGroup = init::create_bind_group(
            m_device,
            m_bindGroupLayout,
            m_uniformBuffer,
            m_atlas.getView(),
            m_atlas.getSampler());

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

            // Perform raycast
            m_selected_block = cast_ray(m_player, m_chunk, 5.0f); // 5.0f is the max distance

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

        m_atlas.cleanup();
        m_chunkMesh.cleanup();

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
