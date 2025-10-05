#include "app.h"
#include <iostream>
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/utils.h"
#include "init/shader.h"
#include "init/buffer.h"
#include "init/pipeline.h"
#include "shader.wgsl.h"
#include "selection_shader.wgsl.h"

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
        m_selectionVertexShader = init::create_shader_module(m_device, "Selection Vertex Shader", WGSL_selection_vertexShaderSource);
        m_selectionFragmentShader = init::create_shader_module(m_device, "Selection Fragment Shader", WGSL_selection_fragmentShaderSource);

        std::cout << "Shaders created successfully" << std::endl;

        std::cout << "Setting up 3D components..." << std::endl;

        m_camera = Camera(
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f},
            (float)m_windowWidth / (float)m_windowHeight,
            45.0f,
            0.1f,
            1000.0f);

        std::cout << "Generating terrain..." << std::endl;
        m_chunk.generateTerrain();
        std::cout << "Generating chunk mesh..." << std::endl;
        m_chunkMesh.generate(m_device, m_chunk);
        std::cout << "Chunk mesh generated." << std::endl;

        m_selectionRenderer.create_buffers(m_device);

        m_uniformBuffer = init::create_uniform_buffer(m_device, "Camera Uniform Buffer", sizeof(CameraUniform));

        std::cout << "3D components ready!" << std::endl;

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

        m_selectionRenderPipeline = init::create_selection_pipeline(
            m_device,
            m_selectionVertexShader,
            m_selectionFragmentShader,
            m_surfaceFormat,
            m_depthTextureFormat,
            &m_selectionBindGroupLayout);

        m_selectionBindGroup = init::create_selection_bind_group(
            m_device,
            m_selectionBindGroupLayout,
            m_uniformBuffer);

        m_running = true;
    }

    void App::run()
    {
        std::cout << "Running app..." << std::endl;

        uint64_t last_tick = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (m_running)
        {
            uint64_t current_tick = SDL_GetPerformanceCounter();
            float dt = static_cast<float>(current_tick - last_tick) / static_cast<float>(SDL_GetPerformanceFrequency());
            last_tick = current_tick;

            while (SDL_PollEvent(&e))
            {
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

            m_player.update(dt, m_chunk);

            m_selectionRenderer.update(std::optional<glm::ivec3>({8, 1, 8}));

            render();
        }
    }

    void App::render()
    {
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

        m_cameraUniform.updateViewProj(m_camera);
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
        {
            WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

            WGPUCommandEncoderDescriptor encoderDesc = {};
            WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

            WGPURenderPassColorAttachment colorAttachment = {};
            colorAttachment.view = textureView;
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f};
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
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

            // Draw the main world chunk
            wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);
            m_chunkMesh.render(renderPass);

            // Draw the selection outline
            wgpuRenderPassEncoderSetPipeline(renderPass, m_selectionRenderPipeline);
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_selectionBindGroup, 0, nullptr);
            m_selectionRenderer.draw(renderPass);

            wgpuRenderPassEncoderEnd(renderPass);

            WGPUCommandBufferDescriptor cmdBufferDesc = {};
            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
            wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

            wgpuSurfacePresent(m_surface);

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

        m_selectionRenderer.cleanup();
        m_atlas.cleanup();
        m_chunkMesh.cleanup();

        if (m_depthTextureView)
            wgpuTextureViewRelease(m_depthTextureView);
        if (m_depthTexture)
            wgpuTextureRelease(m_depthTexture);

        if (m_uniformBuffer)
            wgpuBufferRelease(m_uniformBuffer);

        // Release main pipeline resources
        if (m_bindGroup)
            wgpuBindGroupRelease(m_bindGroup);
        if (m_bindGroupLayout)
            wgpuBindGroupLayoutRelease(m_bindGroupLayout);
        if (m_renderPipeline)
            wgpuRenderPipelineRelease(m_renderPipeline);

        // Release selection pipeline resources
        if (m_selectionBindGroup)
            wgpuBindGroupRelease(m_selectionBindGroup);
        if (m_selectionBindGroupLayout)
            wgpuBindGroupLayoutRelease(m_selectionBindGroupLayout);
        if (m_selectionRenderPipeline)
            wgpuRenderPipelineRelease(m_selectionRenderPipeline);

        // Release shaders
        if (m_vertexShader)
            wgpuShaderModuleRelease(m_vertexShader);
        if (m_fragmentShader)
            wgpuShaderModuleRelease(m_fragmentShader);
        if (m_selectionVertexShader)
            wgpuShaderModuleRelease(m_selectionVertexShader);
        if (m_selectionFragmentShader)
            wgpuShaderModuleRelease(m_selectionFragmentShader);

        if (m_vertexBuffer)
            wgpuBufferRelease(m_vertexBuffer);
        if (m_surface)
            wgpuSurfaceRelease(m_surface);
        if (m_queue)
            wgpuQueueRelease(m_queue);
        if (m_device)
            wgpuDeviceRelease(m_device);
        if (m_adapter)
            wgpuAdapterRelease(m_adapter);
        if (m_instance)
            wgpuInstanceRelease(m_instance);
        if (m_window)
            SDL_DestroyWindow(m_window);

        SDL_Quit();

        m_running = false;
    }

} // namespace flint