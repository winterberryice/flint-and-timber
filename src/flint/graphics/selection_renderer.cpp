#include "selection_renderer.h"
#include "render_pipeline_builder.h"
#include "../init/buffer.h"
#include "../init/shader.h"
#include "../selection_shader.wgsl.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace flint::graphics {

SelectionRenderer::SelectionRenderer() = default;

SelectionRenderer::~SelectionRenderer() = default;

void SelectionRenderer::init(WGPUDevice device,
                             WGPUQueue queue,
                             WGPUTextureFormat surfaceFormat,
                             WGPUTextureFormat depthTextureFormat) {
    std::cout << "Initializing selection renderer..." << std::endl;

    // Create shaders
    m_vertexShader = init::create_shader_module(device, "Selection Vertex Shader", SELECTION_WGSL_vertexShaderSource);
    m_fragmentShader =
        init::create_shader_module(device, "Selection Fragment Shader", SELECTION_WGSL_fragmentShaderSource);

    // Create uniform buffers
    m_cameraUniformBuffer = init::create_uniform_buffer(device, "Camera Uniform Buffer", sizeof(CameraUniform));
    m_modelUniformBuffer = init::create_uniform_buffer(device, "Model Uniform Buffer", sizeof(ModelUniform));

    // Create render pipeline
    m_renderPipeline = RenderPipelineBuilder(device)
                           .with_shaders(m_vertexShader, m_fragmentShader)
                           .with_surface_format(surfaceFormat)
                           .with_depth_format(depthTextureFormat)
                           .with_camera_uniform(m_cameraUniformBuffer)
                           .with_model_uniform(m_modelUniformBuffer)
                           .with_depth_compare(WGPUCompareFunction_LessEqual)
                           .build();

    std::cout << "Selection renderer initialized." << std::endl;
}

void SelectionRenderer::create_mesh(WGPUDevice device) {
    m_selectionMesh.generate(device);
}

void SelectionRenderer::render(WGPURenderPassEncoder renderPass,
                               WGPUQueue queue,
                               const Camera &camera,
                               const std::optional<glm::ivec3> &selected_block_pos) {
    is_visible = selected_block_pos.has_value();

    if (!is_visible) {
        return;
    }

    // Update camera uniform buffer
    m_cameraUniform.updateViewProj(camera);
    wgpuQueueWriteBuffer(queue, m_cameraUniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

    // Update model uniform buffer
    glm::vec3 pos = selected_block_pos.value();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);

    m_modelUniform.model = model;
    wgpuQueueWriteBuffer(queue, m_modelUniformBuffer, 0, &m_modelUniform, sizeof(ModelUniform));

    // Set pipeline and bind group
    wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.get_pipeline());
    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.get_bind_group(), 0, nullptr);

    // Draw the selection mesh
    m_selectionMesh.render(renderPass);
}

void SelectionRenderer::cleanup() {
    std::cout << "Cleaning up selection renderer..." << std::endl;

    m_selectionMesh.cleanup();

    if (m_cameraUniformBuffer) {
        wgpuBufferRelease(m_cameraUniformBuffer);
        m_cameraUniformBuffer = nullptr;
    }
    if (m_modelUniformBuffer) {
        wgpuBufferRelease(m_modelUniformBuffer);
        m_modelUniformBuffer = nullptr;
    }
    if (m_vertexShader) {
        wgpuShaderModuleRelease(m_vertexShader);
        m_vertexShader = nullptr;
    }
    if (m_fragmentShader) {
        wgpuShaderModuleRelease(m_fragmentShader);
        m_fragmentShader = nullptr;
    }
}

} // namespace flint::graphics