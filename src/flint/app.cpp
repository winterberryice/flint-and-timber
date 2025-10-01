#include "app.h"
#include "init/sdl.h"
#include "init/wgpu.h"
#include "init/shader.h"
#include "init/pipeline.h"
#include "shader.wgsl.h"
#include <iostream>
#include <vector>

namespace flint
{
    App::App()
        : m_player(
              {CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f + 10.0f, CHUNK_DEPTH / 2.0f},
              -90.0f, 0.0f, 0.1f)
    {
        std::cout << "Initializing app..." << std::endl;
        m_window = init::sdl(m_windowWidth, m_windowHeight);
        SDL_SetWindowRelativeMouseMode(m_window, true);

        init::wgpu(m_windowWidth, m_windowHeight, m_window, m_instance, m_surface, m_surfaceFormat, m_adapter, m_device, m_queue);

        // --- Load Texture Atlas ---
        m_textureAtlas = graphics::Texture::load_from_atlas_bytes(m_device, m_queue);

        m_vertexShader = init::create_shader_module(m_device, "Vertex Shader", WGSL_vertexShaderSource);
        m_fragmentShader = init::create_shader_module(m_device, "Fragment Shader", WGSL_fragmentShaderSource);

        m_camera = Camera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, (float)m_windowWidth / (float)m_windowHeight, 45.0f, 0.1f, 1000.0f);

        m_chunk.generateTerrain();
        m_chunkMesh.generate(m_device, m_chunk);

        // --- Uniform Buffer ---
        WGPUBufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(CameraUniform);
        uniformBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);

        // --- Create Bind Group Layouts ---
        // Layout for Camera Uniforms (Group 0)
        WGPUBindGroupLayoutEntry uniformLayoutEntry{};
        uniformLayoutEntry.binding = 0;
        uniformLayoutEntry.visibility = WGPUShaderStage_Vertex;
        uniformLayoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
        WGPUBindGroupLayoutDescriptor uniformBGLDesc{};
        uniformBGLDesc.entryCount = 1;
        uniformBGLDesc.entries = &uniformLayoutEntry;
        m_uniformBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &uniformBGLDesc);

        // Layout for Texture Atlas (Group 1)
        std::vector<WGPUBindGroupLayoutEntry> textureLayoutEntries(2);
        // t_atlas (texture)
        textureLayoutEntries[0].binding = 0;
        textureLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
        textureLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
        textureLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
        // s_atlas (sampler)
        textureLayoutEntries[1].binding = 1;
        textureLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
        textureLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;
        WGPUBindGroupLayoutDescriptor textureBGLDesc{};
        textureBGLDesc.entryCount = textureLayoutEntries.size();
        textureBGLDesc.entries = textureLayoutEntries.data();
        m_textureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &textureBGLDesc);

        // --- Create Depth Texture ---
        WGPUTextureDescriptor depthTextureDesc{};
        depthTextureDesc.dimension = WGPUTextureDimension_2D;
        depthTextureDesc.format = m_depthTextureFormat;
        depthTextureDesc.size = { (uint32_t)m_windowWidth, (uint32_t)m_windowHeight, 1 };
        depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
        m_depthTexture = wgpuDeviceCreateTexture(m_device, &depthTextureDesc);
        WGPUTextureViewDescriptor depthViewDesc{};
        depthViewDesc.format = m_depthTextureFormat;
        depthViewDesc.dimension = WGPUTextureViewDimension_2D;
        depthViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        m_depthTextureView = wgpuTextureCreateView(m_depthTexture, &depthViewDesc);

        // --- Create Render Pipeline ---
        std::vector<WGPUBindGroupLayout> bindGroupLayouts = {m_uniformBindGroupLayout, m_textureBindGroupLayout};
        m_renderPipeline = init::create_render_pipeline(m_device, m_vertexShader, m_fragmentShader, m_surfaceFormat, m_depthTextureFormat, bindGroupLayouts);

        // --- Create Bind Groups ---
        // Uniform Bind Group
        WGPUBindGroupEntry uniformBindGroupEntry{};
        uniformBindGroupEntry.binding = 0;
        uniformBindGroupEntry.buffer = m_uniformBuffer;
        uniformBindGroupEntry.size = sizeof(CameraUniform);
        WGPUBindGroupDescriptor uniformBGDesc{};
        uniformBGDesc.layout = m_uniformBindGroupLayout;
        uniformBGDesc.entryCount = 1;
        uniformBGDesc.entries = &uniformBindGroupEntry;
        m_uniformBindGroup = wgpuDeviceCreateBindGroup(m_device, &uniformBGDesc);

        // Texture Bind Group
        std::vector<WGPUBindGroupEntry> textureBindGroupEntries(2);
        textureBindGroupEntries[0].binding = 0;
        textureBindGroupEntries[0].textureView = m_textureAtlas.get_view();
        textureBindGroupEntries[1].binding = 1;
        textureBindGroupEntries[1].sampler = m_textureAtlas.get_sampler();
        WGPUBindGroupDescriptor textureBGDesc{};
        textureBGDesc.layout = m_textureBindGroupLayout;
        textureBGDesc.entryCount = textureBindGroupEntries.size();
        textureBGDesc.entries = textureBindGroupEntries.data();
        m_textureBindGroup = wgpuDeviceCreateBindGroup(m_device, &textureBGDesc);

        m_running = true;
    }

    void App::run()
    {
        uint64_t last_tick = SDL_GetPerformanceCounter();
        while (m_running)
        {
            uint64_t current_tick = SDL_GetPerformanceCounter();
            float dt = (float)(current_tick - last_tick) / (float)SDL_GetPerformanceFrequency();
            last_tick = current_tick;

            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                m_player.handle_input(e);
                if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE))
                    m_running = false;
                if (e.type == SDL_EVENT_MOUSE_MOTION)
                    m_player.process_mouse_movement((float)e.motion.xrel, (float)e.motion.yrel);
            }
            m_player.update(dt, m_chunk);
            render();
        }
    }

    void App::render()
    {
        m_camera.eye = m_player.get_position() + glm::vec3(0.0f, physics::PLAYER_EYE_HEIGHT, 0.0f);
        float yaw_rad = glm::radians(m_player.get_yaw());
        float pitch_rad = glm::radians(m_player.get_pitch());
        glm::vec3 front;
        front.x = cos(yaw_rad) * cos(pitch_rad);
        front.y = sin(pitch_rad);
        front.z = sin(yaw_rad) * cos(pitch_rad);
        m_camera.target = m_camera.eye + glm::normalize(front);

        m_cameraUniform.updateViewProj(m_camera);
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) return;

        WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

        WGPURenderPassColorAttachment colorAttachment{};
        colorAttachment.view = textureView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f};

        WGPURenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.view = m_depthTextureView;
        depthAttachment.depthClearValue = 1.0f;
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Store;

        WGPURenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthAttachment;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline);

        // Set both bind groups
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_uniformBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 1, m_textureBindGroup, 0, nullptr);

        m_chunkMesh.render(renderPass);
        wgpuRenderPassEncoderEnd(renderPass);

        WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_queue, 1, &cmdBuffer);

        wgpuSurfacePresent(m_surface);

        wgpuTextureViewRelease(textureView);
        wgpuRenderPassEncoderRelease(renderPass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmdBuffer);
        wgpuTextureRelease(surfaceTexture.texture);
    }

    App::~App()
    {
        std::cout << "Terminating app..." << std::endl;
        m_chunkMesh.cleanup();
        if (m_depthTextureView) wgpuTextureViewRelease(m_depthTextureView);
        if (m_depthTexture) wgpuTextureRelease(m_depthTexture);
        if (m_uniformBuffer) wgpuBufferRelease(m_uniformBuffer);
        if (m_uniformBindGroup) wgpuBindGroupRelease(m_uniformBindGroup);
        if (m_uniformBindGroupLayout) wgpuBindGroupLayoutRelease(m_uniformBindGroupLayout);
        if (m_textureBindGroup) wgpuBindGroupRelease(m_textureBindGroup);
        if (m_textureBindGroupLayout) wgpuBindGroupLayoutRelease(m_textureBindGroupLayout);
        if (m_renderPipeline) wgpuRenderPipelineRelease(m_renderPipeline);
        if (m_vertexShader) wgpuShaderModuleRelease(m_vertexShader);
        if (m_fragmentShader) wgpuShaderModuleRelease(m_fragmentShader);
        if (m_surface) wgpuSurfaceRelease(m_surface);
        if (m_queue) wgpuQueueRelease(m_queue);
        if (m_device) wgpuDeviceRelease(m_device);
        if (m_adapter) wgpuAdapterRelease(m_adapter);
        if (m_instance) wgpuInstanceRelease(m_instance);
        if (m_window) SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
} // namespace flint