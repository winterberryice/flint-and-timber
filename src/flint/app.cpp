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

        m_worldRenderer.init(m_device, m_queue, m_surfaceFormat, m_depthTextureFormat);
        m_worldRenderer.generateChunk(m_device);

        m_selectionRenderer.init(m_device, m_queue, m_surfaceFormat, m_depthTextureFormat);
        m_selectionRenderer.create_mesh(m_device);

        m_crosshair.init(m_device, m_queue, m_surfaceFormat, m_windowWidth, m_windowHeight);

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
                else if (e.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    resize(e.window.data1, e.window.data2);
                }
                else if (e.type == SDL_EVENT_MOUSE_MOTION)
                {
                    m_player.process_mouse_movement(static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel));
                }
            }

            // Update player physics and state
            m_player.update(dt, m_worldRenderer.getChunk());

            // Render the scene
            render();
        }
    }

void App::resize(int width, int height)
{
    std::cout << "Resizing to " << width << "x" << height << std::endl;

    m_windowWidth = width;
    m_windowHeight = height;

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

    // Update crosshair projection
    m_crosshair.resize(m_windowWidth, m_windowHeight, m_queue);

    // Reconfigure the surface with the new size
    WGPUSurfaceConfiguration surface_config = {};
    surface_config.format = m_surfaceFormat;
    surface_config.usage = WGPUTextureUsage_RenderAttachment;
    surface_config.width = m_windowWidth;
    surface_config.height = m_windowHeight;
    surface_config.presentMode = WGPUPresentMode_Fifo;
    surface_config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(m_surface, &surface_config);
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

    void App::render()
    {
        update_camera();

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

            WGPURenderPassEncoder renderPass = init::begin_render_pass(encoder, textureView, m_depthTextureView);

            m_worldRenderer.render(renderPass, m_queue, m_camera);

            // Get selected block from player and render selection box
            auto selected_block = m_player.get_selected_block();
            std::optional<glm::ivec3> selected_block_pos;
            if (selected_block.has_value())
            {
                selected_block_pos = selected_block->block_position;
            }
            m_selectionRenderer.render(renderPass, m_queue, m_camera, selected_block_pos);

            m_crosshair.draw(renderPass);

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

        m_crosshair.cleanup();
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
