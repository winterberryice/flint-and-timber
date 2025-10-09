#include "crosshair_renderer.h"
#include "render_pipeline_builder.h"
#include "../init/shader.h"
#include "../crosshair_shader.wgsl.h"
#include <iostream>

namespace flint::graphics {

CrosshairRenderer::CrosshairRenderer() = default;

CrosshairRenderer::~CrosshairRenderer() = default;

void CrosshairRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, float aspectRatio) {
    std::cout << "Initializing crosshair renderer..." << std::endl;

    // Create shaders
    m_vertexShader = init::create_shader_module(device, "Crosshair Vertex Shader", CROSSHAIR_WGSL_vertexShaderSource);
    m_fragmentShader = init::create_shader_module(device, "Crosshair Fragment Shader", CROSSHAIR_WGSL_fragmentShaderSource);

    // Create render pipeline
    m_renderPipeline = RenderPipelineBuilder(device)
                           .with_shaders(m_vertexShader, m_fragmentShader)
                           .with_surface_format(surfaceFormat)
                           .build();

    // Create the crosshair mesh
    m_crosshairMesh.generate(device, aspectRatio);

    std::cout << "Crosshair renderer initialized." << std::endl;
}

void CrosshairRenderer::render(WGPURenderPassEncoder renderPass) {
    // Set pipeline and draw the crosshair
    wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.get_pipeline());
    m_crosshairMesh.render(renderPass);
}

void CrosshairRenderer::cleanup() {
    std::cout << "Cleaning up crosshair renderer..." << std::endl;

    m_crosshairMesh.cleanup();

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