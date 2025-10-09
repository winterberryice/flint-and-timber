#include "crosshair_renderer.h"

#include <iostream>

#include "../init/shader.h"
#include "../crosshair_shader.wgsl.h"

namespace flint::graphics
{

    CrosshairRenderer::CrosshairRenderer() = default;

    CrosshairRenderer::~CrosshairRenderer() = default;

    void CrosshairRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, float aspectRatio)
    {
        std::cout << "Initializing crosshair renderer..." << std::endl;

        // Create shaders
        m_vertexShader = init::create_shader_module(device, "Crosshair Vertex Shader", CROSSHAIR_WGSL_vertexShaderSource.data());
        m_fragmentShader = init::create_shader_module(device, "Crosshair Fragment Shader", CROSSHAIR_WGSL_fragmentShaderSource.data());

        // Create render pipeline
        m_renderPipeline.init(
            device,
            m_vertexShader,
            m_fragmentShader,
            surfaceFormat,
            WGPUTextureFormat_Undefined, // No depth texture
            nullptr,                     // No camera uniform buffer
            nullptr,                     // No model uniform buffer
            nullptr,                     // No texture view
            nullptr,                     // No sampler
            false,                       // Do not use texture
            false,                       // Do not use model matrix
            false,                       // Do not write to depth buffer
            WGPUCompareFunction_Always,  // Depth test always passes
            false,                       // No blending
            false,                       // No culling
            WGPUPrimitiveTopology_TriangleList, // Use TriangleList to render quads
            true                         // Is UI
        );

        // Create the crosshair mesh
        m_crosshairMesh.generate(device, aspectRatio);

        std::cout << "Crosshair renderer initialized." << std::endl;
    }

    void CrosshairRenderer::render(WGPURenderPassEncoder renderPass)
    {
        // Set pipeline and draw the crosshair
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.getPipeline());
        m_crosshairMesh.render(renderPass);
    }

    void CrosshairRenderer::cleanup()
    {
        std::cout << "Cleaning up crosshair renderer..." << std::endl;

        m_renderPipeline.cleanup();
        m_crosshairMesh.cleanup();

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