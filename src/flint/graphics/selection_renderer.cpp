#include "selection_renderer.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

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

        // Create uniform buffers
        m_cameraUniformBuffer = init::create_uniform_buffer(device, "Camera Uniform Buffer", sizeof(CameraUniform));
        m_modelUniformBuffer = init::create_uniform_buffer(device, "Model Uniform Buffer", sizeof(ModelUniform));

        // Create render pipeline
        m_renderPipeline.init(
            device,
            m_vertexShader,
            m_fragmentShader,
            surfaceFormat,
            depthTextureFormat,
            m_cameraUniformBuffer,
            m_modelUniformBuffer,
            nullptr, // No texture view
            nullptr, // No sampler
            WGPUPrimitiveTopology_LineList,
            false,   // Do not use texture
            false,   // Do not write to depth buffer
            WGPUCompareFunction_LessEqual
        );

        std::cout << "Selection renderer initialized." << std::endl;
    }

    void SelectionRenderer::generateSelectionBox(WGPUDevice device)
    {
        // Generate a simple 1x1x1 cube at the origin.
        // Its position will be set by the model matrix uniform.
        m_selectionMesh.generate(device);
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera, const std::optional<glm::ivec3> &selected_block)
    {
        if (!selected_block.has_value())
        {
            return; // Don't render if no block is selected
        }

        // Update camera uniform buffer
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_cameraUniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Update model uniform buffer
        glm::vec3 block_pos_f = selected_block.value();
        glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), block_pos_f);
        m_modelUniform.model = model_matrix;
        wgpuQueueWriteBuffer(queue, m_modelUniformBuffer, 0, &m_modelUniform, sizeof(ModelUniform));

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

        if (m_modelUniformBuffer)
        {
            wgpuBufferRelease(m_modelUniformBuffer);
            m_modelUniformBuffer = nullptr;
        }
        if (m_cameraUniformBuffer)
        {
            wgpuBufferRelease(m_cameraUniformBuffer);
            m_cameraUniformBuffer = nullptr;
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