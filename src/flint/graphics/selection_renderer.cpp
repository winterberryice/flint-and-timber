#include "selection_renderer.h"

#include <iostream>

#include "../init/buffer.h"
#include "../init/shader.h"
#include "../selection_shader.wgsl.h"
#include "../chunk.h"

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
            nullptr, // No sampler
            false    // Do not use texture
        );

        std::cout << "Selection renderer initialized." << std::endl;
    }

    void SelectionRenderer::generateSelectionBox(WGPUDevice device)
    {
        // Hardcoded position for the cube for now, raised slightly for visibility
        glm::vec3 position(7.0f, 19.0f, 7.0f);
        m_selectionMesh.generate(device, position);
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera)
    {
        // Update the uniform buffer with the new camera view-projection matrix
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Set pipeline and bind group
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.getPipeline());
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.getBindGroup(), 0, nullptr);

        // Draw the selection mesh
        m_selectionMesh.render(renderPass);
    }

    void SelectionRenderer::cleanup()
    {
        std::cout << "Cleaning up selection renderer..." << std::endl;

        m_renderPipeline.cleanup();
        m_selectionMesh.cleanup();

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