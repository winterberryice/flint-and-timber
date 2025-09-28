#include "app.h"
#include <iostream>
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/utils.h"
#include "init/shader.h"
#include "init/buffer.h"
#include "init/pipeline.h"
#include "shader.wgsl.h"

namespace flint
{
    App::App()
    {
        std::cout << "Initializing app..." << std::endl;
        m_windowWidth = 800;
        m_windowHeight = 600;
        m_window = init::sdl(m_windowWidth, m_windowHeight);

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

        m_camera = Camera(
            {CHUNK_WIDTH * 1.5f, CHUNK_HEIGHT * 0.7f, CHUNK_DEPTH * 1.5f}, // eye
            {CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f, CHUNK_DEPTH / 2.0f}, // target
            {0.0f, 1.0f, 0.0f},                                            // up
            (float)m_windowWidth / (float)m_windowHeight,                  // aspect
            45.0f,                                                         // fovy
            0.1f,                                                          // znear
            1000.0f                                                        // zfar
        );
        m_cameraController = CameraController(15.0f, 0.003f);

        // Generate terrain and mesh
        std::cout << "Generating terrain..." << std::endl;
        m_chunk.generateTerrain();
        std::cout << "Generating chunk mesh..." << std::endl;
        m_chunkMesh.generate(m_device, m_chunk);
        std::cout << "Chunk mesh generated." << std::endl;

        // Create uniform buffer for camera matrices
        m_uniformBuffer = init::create_uniform_buffer(m_device, "Camera Uniform Buffer", sizeof(CameraUniform));

        std::cout << "3D components ready!" << std::endl;

        m_renderPipeline = init::create_render_pipeline(
            m_device,
            m_vertexShader,
            m_fragmentShader,
            m_surfaceFormat,
            &m_bindGroupLayout);

        m_bindGroup = init::create_bind_group(m_device, m_bindGroupLayout, m_uniformBuffer);

        // ====
        m_running = true;
    }

    void App::run()
    {
        std::cout << "Running app..." << std::endl;

        SDL_Event e;
        while (m_running)
        {
            // Handle events
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_EVENT_QUIT)
                {
                    std::cout << "Quit event received" << std::endl;
                    m_running = false;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                {
                    std::cout << "Escape key pressed" << std::endl;
                    m_running = false;
                }
            }

            // Render
            render();
        }
    }

    void App::render()
    {
        float dt = 1.0f / 60.0f;
        // m_cameraController.updateCamera(m_camera, dt); // static camera for now (no controller)
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

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.nextInChain = nullptr;
            renderPassDesc.label = {nullptr, 0};
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            renderPassDesc.depthStencilAttachment = nullptr;

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

        m_chunkMesh.cleanup();

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
