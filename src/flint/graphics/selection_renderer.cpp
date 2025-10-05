#include "selection_renderer.h"

#include <iostream>

#include "../init/buffer.h"
#include "../init/shader.h"
#include "../selection_shader.wgsl.h"
#include "../cube_geometry.h"

namespace flint::graphics
{

    SelectionRenderer::SelectionRenderer() = default;

    SelectionRenderer::~SelectionRenderer() = default;

    void SelectionRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat)
    {
        std::cout << "Initializing selection renderer..." << std::endl;

        // Create shaders
        m_vertexShader = init::create_shader_module(device, "Selection Vertex Shader", SELECTION_WGSL_vertexShaderSource.data());
        m_fragmentShader = init::create_shader_module(device, "Selection Fragment Shader", SELECTION_WGSL_fragmentShaderSource.data());

        // Create uniform buffer for camera matrices
        m_uniformBuffer = init::create_uniform_buffer(device, "Camera Uniform Buffer", sizeof(CameraUniform));

        // Create render pipeline
        m_renderPipeline.init(
            device,
            m_vertexShader,
            m_fragmentShader,
            surfaceFormat,
            depthTextureFormat,
            m_uniformBuffer,
            nullptr, // No texture view
            nullptr  // No sampler
        );

        createCubeMesh(device);

        std::cout << "Selection renderer initialized." << std::endl;
    }

    void SelectionRenderer::createCubeMesh(WGPUDevice device)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // Hardcoded position for the cube for now
        glm::vec3 position(CHUNK_WIDTH / 2.0f, CHUNK_HEIGHT / 2.0f, CHUNK_DEPTH / 2.0f);

        // Get cube geometry
        auto cube_vertices = flint::get_cube_vertices();
        auto cube_indices = flint::get_cube_indices();

        for (const auto &vert : cube_vertices)
        {
            vertices.push_back({vert.position + position, vert.normal, vert.uv});
        }

        for (const auto &index : cube_indices)
        {
            indices.push_back(index);
        }

        m_indexCount = static_cast<uint32_t>(indices.size());

        // Create vertex buffer
        m_vertexBuffer = init::create_vertex_buffer(device, "Selection Vertex Buffer", vertices.data(), vertices.size() * sizeof(Vertex));

        // Create index buffer
        m_indexBuffer = init::create_index_buffer(device, "Selection Index Buffer", indices.data(), indices.size() * sizeof(uint32_t));
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera)
    {
        // Update the uniform buffer with the new camera view-projection matrix
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Set pipeline and bind group
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.getPipeline());
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.getBindGroup(), 0, nullptr);

        // Set vertex and index buffers
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

        // Draw the cube
        wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
    }

    void SelectionRenderer::cleanup()
    {
        std::cout << "Cleaning up selection renderer..." << std::endl;

        m_renderPipeline.cleanup();

        if (m_vertexBuffer)
        {
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer)
        {
            wgpuBufferRelease(m_indexBuffer);
            m_indexBuffer = nullptr;
        }
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