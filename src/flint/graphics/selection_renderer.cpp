#include "selection_renderer.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "../init/buffer.h"
#include "../init/shader.h"
#include "../selection_shader.wgsl.h"

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
            false,   // Do not use texture
            true,    // Use model matrix
            false,   // Do not write to depth buffer
            WGPUCompareFunction_LessEqual,
            WGPUPrimitiveTopology_TriangleList,
            false); // No blending for the opaque highlight

        std::cout << "Selection renderer initialized." << std::endl;
    }

    void SelectionRenderer::create_mesh(WGPUDevice device)
    {
        m_selectionMesh.generate(device);
    }

    // Helper to get rotation for a face normal
    glm::mat4 get_rotation_for_normal(const glm::ivec3 &normal)
    {
        if (normal == glm::ivec3(0, 0, 1))
            return glm::mat4(1.0f); // Front
        if (normal == glm::ivec3(0, 0, -1))
            return glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), {0.0f, 1.0f, 0.0f}); // Back
        if (normal == glm::ivec3(1, 0, 0))
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), {0.0f, 1.0f, 0.0f}); // Right
        if (normal == glm::ivec3(-1, 0, 0))
            return glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), {0.0f, 1.0f, 0.0f}); // Left
        if (normal == glm::ivec3(0, 1, 0))
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), {1.0f, 0.0f, 0.0f}); // Top
        if (normal == glm::ivec3(0, -1, 0))
            return glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), {1.0f, 0.0f, 0.0f}); // Bottom
        return glm::mat4(1.0f);
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera, const std::optional<glm::ivec3> &selected_block_pos, const std::vector<glm::ivec3> &exposed_faces)
    {
        is_visible = selected_block_pos.has_value() && !exposed_faces.empty();

        if (!is_visible)
        {
            return;
        }

        // Update camera uniform buffer once
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_cameraUniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Set pipeline and bind group once
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.getPipeline());
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.getBindGroup(), 0, nullptr);

        glm::vec3 block_center = glm::vec3(selected_block_pos.value()) + glm::vec3(0.5f);

        for (const auto &normal : exposed_faces)
        {
            glm::mat4 rotation_matrix = get_rotation_for_normal(normal);
            glm::vec3 face_center = block_center + (glm::vec3(normal) * 0.501f); // 0.5 to reach face, 0.001 to prevent Z-fighting

            glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), face_center) * rotation_matrix;

            m_modelUniform.model = model_matrix;
            wgpuQueueWriteBuffer(queue, m_modelUniformBuffer, 0, &m_modelUniform, sizeof(ModelUniform));

            // Draw the border mesh for this face
            m_selectionMesh.render(renderPass);
        }
    }

    void SelectionRenderer::cleanup()
    {
        std::cout << "Cleaning up selection renderer..." << std::endl;

        m_renderPipeline.cleanup();
        m_selectionMesh.cleanup();

        if (m_cameraUniformBuffer)
        {
            wgpuBufferRelease(m_cameraUniformBuffer);
            m_cameraUniformBuffer = nullptr;
        }
        if (m_modelUniformBuffer)
        {
            wgpuBufferRelease(m_modelUniformBuffer);
            m_modelUniformBuffer = nullptr;
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