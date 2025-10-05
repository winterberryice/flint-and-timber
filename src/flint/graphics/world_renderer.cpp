#include "world_renderer.h"

#include <iostream>

#include "../vertex.h"
#include "atlas_bytes.hpp"
#include "../init/buffer.h"
#include "../init/shader.h"
#include "../shader.wgsl.h"

namespace flint::graphics
{

    WorldRenderer::WorldRenderer() = default;

    WorldRenderer::~WorldRenderer() = default;

    void WorldRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat)
    {
        std::cout << "Initializing world renderer..." << std::endl;

        // Load texture atlas
        if (!m_atlas.loadFromMemory(device, queue, assets_textures_block_atlas_png, assets_textures_block_atlas_png_len))
        {
            std::cerr << "Failed to load texture atlas for world renderer!" << std::endl;
            // In a real app, you might want to throw an exception here.
        }

        // Create shaders
        m_vertexShader = init::create_shader_module(device, "World Vertex Shader", WGSL_vertexShaderSource);
        m_fragmentShader = init::create_shader_module(device, "World Fragment Shader", WGSL_fragmentShaderSource);

        // Create uniform buffer for camera matrices
        m_uniformBuffer = init::create_uniform_buffer(device, "Camera Uniform Buffer", sizeof(CameraUniform));

        // Create render pipeline
        WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();
        m_renderPipeline.init(
            device,
            m_vertexShader,
            m_fragmentShader,
            surfaceFormat,
            depthTextureFormat,
            m_uniformBuffer,
            m_atlas.getView(),
            m_atlas.getSampler(),
            &vertexBufferLayout,
            true);

        std::cout << "World renderer initialized." << std::endl;
    }

    void WorldRenderer::generateChunk(WGPUDevice device)
    {
        std::cout << "Generating terrain..." << std::endl;
        m_chunk.generateTerrain();
        std::cout << "Generating chunk mesh..." << std::endl;
        m_chunkMesh.generate(device, m_chunk);
        std::cout << "Chunk mesh generated." << std::endl;
    }

    Chunk &WorldRenderer::getChunk()
    {
        return m_chunk;
    }

    const Chunk &WorldRenderer::getChunk() const
    {
        return m_chunk;
    }

    void WorldRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera)
    {
        // Update the uniform buffer with the new camera view-projection matrix
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Set pipeline and bind group
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.getPipeline());
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.getBindGroup(), 0, nullptr);

        // Draw the chunk
        m_chunkMesh.render(renderPass);
    }

    void WorldRenderer::cleanup()
    {
        std::cout << "Cleaning up world renderer..." << std::endl;

        m_renderPipeline.cleanup();
        m_atlas.cleanup();
        m_chunkMesh.cleanup();

        if (m_uniformBuffer)
        {
            wgpuBufferRelease(m_uniformBuffer);
            m_uniformBuffer = nullptr;
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
    }

} // namespace flint::graphics