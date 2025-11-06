#include "app.h"
#include <iostream>
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/utils.h"
#include "init/shader.h"
#include "init/buffer.h"
#include "init/pipeline.h"
#include "init/texture.h"
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
        m_windowWidth = 1280;
        m_windowHeight = 720;
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

        m_worldRenderer.init(m_device, m_queue, m_surfaceFormat, m_depthTextureFormat);

        m_selectionRenderer.init(m_device, m_queue, m_surfaceFormat, m_depthTextureFormat);
        m_selectionRenderer.create_mesh(m_device);

        m_crosshairRenderer.init(m_device, m_queue, m_surfaceFormat, m_windowWidth, m_windowHeight);

        m_debugScreenRenderer.init(m_window, m_device, m_surfaceFormat);

        // The camera is now controlled by the player, so we initialize it with placeholder values.
        // It will be updated every frame in the `render` loop based on the player's state.
        m_camera = Camera(
            {0.0f, 0.0f, 0.0f},                           // eye (will be overwritten)
            {0.0f, 0.0f, -1.0f},                          // target (will be overwritten)
            {0.0f, 1.0f, 0.0f},                           // up
            (float)m_windowWidth / (float)m_windowHeight, // aspect
            m_initialFovY,                                // fovy
            0.1f,                                         // znear
            1000.0f                                       // zfar
        );

        init::create_depth_texture(
            m_device,
            m_windowWidth,
            m_windowHeight,
            m_depthTextureFormat,
            &m_depthTexture,
            &m_depthTextureView);

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
                // Let debug screen renderer process the event first
                m_debugScreenRenderer.process_event(e);

                // Let the player handle keyboard input
                m_player.handle_input(e);

                if (e.type == SDL_EVENT_QUIT)
                {
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    if (SDL_GetWindowRelativeMouseMode(m_window))
                    {
                        SDL_SetWindowRelativeMouseMode(m_window, false);
                    }
                    else
                    {
                        m_running = false;
                    }
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_F3)
                {
                    m_showDebugScreen = !m_showDebugScreen;
                }
                else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                    onResize(e.window.data1, e.window.data2);
                }
                else if (e.type == SDL_EVENT_MOUSE_MOTION)
                {
                    if (SDL_GetWindowRelativeMouseMode(m_window))
                    {
                        m_player.process_mouse_movement(static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel));
                    }
                }
                else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    if (!SDL_GetWindowRelativeMouseMode(m_window))
                    {
                        SDL_SetWindowRelativeMouseMode(m_window, true);
                        continue;
                    }

                    if (m_player.on_mouse_click(e.button, m_worldRenderer.getWorld()))
                    {
                        m_worldRenderer.rebuild_chunk_mesh(m_device);
                    }
                }
            }

            // Update player physics and state
            m_player.update(dt, m_worldRenderer.getWorld());

            // Render the scene
            render();
        }
    }

    void App::update_camera()
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
    }

    void App::onResize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;

        // Reconfigure the surface
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.device = m_device;
        surfaceConfig.format = m_surfaceFormat;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        surfaceConfig.viewFormatCount = 0;
        surfaceConfig.viewFormats = nullptr;
        wgpuSurfaceConfigure(m_surface, &surfaceConfig);

        // Re-create depth texture
        if (m_depthTextureView)
            wgpuTextureViewRelease(m_depthTextureView);
        if (m_depthTexture)
            wgpuTextureRelease(m_depthTexture);
        init::create_depth_texture(
            m_device,
            m_windowWidth,
            m_windowHeight,
            m_depthTextureFormat,
            &m_depthTexture,
            &m_depthTextureView);

        // Update camera aspect ratio
        m_camera.aspect = (float)m_windowWidth / (float)m_windowHeight;
        m_camera.fovy_rads = glm::radians(m_initialFovY);

        // Update crosshair
        m_crosshairRenderer.onResize(m_windowWidth, m_windowHeight);
    }

    void App::render()
    {
        update_camera();

        // Begin debug screen frame (if enabled)
        if (m_showDebugScreen)
        {
            m_debugScreenRenderer.begin_frame(m_player, m_worldRenderer.getWorld());
        }

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

            // --- Main 3D Render Pass ---
            WGPURenderPassEncoder renderPass = init::begin_render_pass(encoder, textureView, m_depthTextureView);
            m_worldRenderer.render(renderPass, m_queue, m_camera);
            auto selected_block = m_player.get_selected_block();
            std::optional<glm::ivec3> selected_block_pos;
            if (selected_block.has_value())
            {
                selected_block_pos = selected_block->block_position;
            }
            m_selectionRenderer.render(renderPass, m_queue, m_camera, selected_block_pos);
            wgpuRenderPassEncoderEnd(renderPass);

            // --- UI Overlay Render Pass ---
            WGPURenderPassEncoder overlayRenderPass = init::begin_overlay_render_pass(encoder, textureView);
            m_crosshairRenderer.render(overlayRenderPass);
            if (m_showDebugScreen)
            {
                m_debugScreenRenderer.render(overlayRenderPass);
            }
            wgpuRenderPassEncoderEnd(overlayRenderPass);

            WGPUCommandBufferDescriptor cmdBufferDesc = {};
            cmdBufferDesc.nextInChain = nullptr;
            cmdBufferDesc.label = {nullptr, 0};

            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
            wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

            wgpuSurfacePresent(m_surface);

            // Cleanup
            wgpuCommandBufferRelease(cmdBuffer);
            wgpuRenderPassEncoderRelease(overlayRenderPass);
            wgpuRenderPassEncoderRelease(renderPass);
            wgpuCommandEncoderRelease(encoder);
            wgpuTextureViewRelease(textureView);
        }

        wgpuTextureRelease(surfaceTexture.texture);
    }

    App::~App()
    {
        std::cout << "Terminating app..." << std::endl;

        m_debugScreenRenderer.cleanup();
        m_crosshairRenderer.cleanup();
        m_selectionRenderer.cleanup();
        m_worldRenderer.cleanup();

        if (m_depthTextureView)
            wgpuTextureViewRelease(m_depthTextureView);
        if (m_depthTexture)
            wgpuTextureRelease(m_depthTexture);

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
