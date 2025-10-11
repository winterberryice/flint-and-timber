#include "crosshair_renderer.h"

#include <iostream>
#include <vector>

#include "../init/shader.h"
#include "../init/utils.h"
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

        // Create Render Pipeline
        {
            // The crosshair is a UI element, so it doesn't need a bind group layout.
            m_renderPipeline.bindGroupLayout = nullptr;

            // Create pipeline layout
            WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
            pipelineLayoutDesc.bindGroupLayoutCount = 0;
            pipelineLayoutDesc.bindGroupLayouts = nullptr;
            WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

            // Vertex layout for UI
            std::vector<WGPUVertexAttribute> vertexAttributes(1);
            // position
            vertexAttributes[0].format = WGPUVertexFormat_Float32x2;
            vertexAttributes[0].offset = 0;
            vertexAttributes[0].shaderLocation = 0;

            WGPUVertexBufferLayout vertexBufferLayout = {};
            vertexBufferLayout.arrayStride = 2 * sizeof(float);
            vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
            vertexBufferLayout.attributeCount = vertexAttributes.size();
            vertexBufferLayout.attributes = vertexAttributes.data();

            // Create render pipeline descriptor
            WGPURenderPipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor.label = init::makeStringView("Crosshair Render Pipeline");

            // Vertex state
            pipelineDescriptor.vertex.module = m_vertexShader;
            pipelineDescriptor.vertex.entryPoint = init::makeStringView("vs_main");
            pipelineDescriptor.vertex.bufferCount = 1;
            pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

            // Fragment state
            WGPUFragmentState fragmentState = {};
            fragmentState.module = m_fragmentShader;
            fragmentState.entryPoint = init::makeStringView("fs_main");

            // Color target
            WGPUColorTargetState colorTarget = {};
            colorTarget.format = surfaceFormat;
            colorTarget.writeMask = WGPUColorWriteMask_All;
            colorTarget.blend = nullptr; // No blending

            fragmentState.targetCount = 1;
            fragmentState.targets = &colorTarget;
            pipelineDescriptor.fragment = &fragmentState;

            // Primitive state
            pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
            pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
            pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
            pipelineDescriptor.primitive.cullMode = WGPUCullMode_None;

            // No depth stencil
            pipelineDescriptor.depthStencil = nullptr;

            // Multisample state
            pipelineDescriptor.multisample.count = 1;
            pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
            pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

            pipelineDescriptor.layout = pipelineLayout;

            m_renderPipeline.pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

            wgpuPipelineLayoutRelease(pipelineLayout);
        }

        // The crosshair has no uniforms, so the bind group is not needed.
        m_renderPipeline.bindGroup = nullptr;

        // Create the crosshair mesh
        m_crosshairMesh.generate(device, aspectRatio);

        std::cout << "Crosshair renderer initialized." << std::endl;
    }

    void CrosshairRenderer::render(WGPURenderPassEncoder renderPass)
    {
        // Set pipeline and draw the crosshair
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.pipeline);
        m_crosshairMesh.render(renderPass);
    }

    void CrosshairRenderer::updateAspectRatio(float aspectRatio)
    {
        // The device is not available here. The mesh needs to be regenerated.
        // This is a bit of a hack, but it's the simplest way to do it without a major refactor.
        m_crosshairMesh.updateAspectRatio(aspectRatio);
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