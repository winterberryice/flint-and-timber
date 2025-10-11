#include "selection_renderer.h"

#include <iostream>
#include <vector>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "../init/buffer.h"
#include "../init/shader.h"
#include "../init/utils.h"
#include "../selection_shader.wgsl.h"
#include "../vertex.h"


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

        // Create Render Pipeline
        {
            // Create bind group layout
            std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries;

            // Binding 0: Camera Uniform Buffer (Vertex)
            WGPUBindGroupLayoutEntry cameraUniformEntry = {};
            cameraUniformEntry.binding = 0;
            cameraUniformEntry.visibility = WGPUShaderStage_Vertex;
            cameraUniformEntry.buffer.type = WGPUBufferBindingType_Uniform;
            cameraUniformEntry.buffer.minBindingSize = sizeof(CameraUniform);
            bindingLayoutEntries.push_back(cameraUniformEntry);

            // Binding 1: Model Uniform Buffer (Vertex)
            WGPUBindGroupLayoutEntry modelUniformEntry = {};
            modelUniformEntry.binding = 1;
            modelUniformEntry.visibility = WGPUShaderStage_Vertex;
            modelUniformEntry.buffer.type = WGPUBufferBindingType_Uniform;
            modelUniformEntry.buffer.minBindingSize = sizeof(ModelUniform);
            bindingLayoutEntries.push_back(modelUniformEntry);

            WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
            bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();
            bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
            m_renderPipeline.bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

            // Create pipeline layout
            WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
            pipelineLayoutDesc.bindGroupLayoutCount = 1;
            pipelineLayoutDesc.bindGroupLayouts = &m_renderPipeline.bindGroupLayout;
            WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

            // Vertex layout
            WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

            // Depth Stencil State
            WGPUDepthStencilState depthStencilState = {};
            depthStencilState.format = depthTextureFormat;
            depthStencilState.depthWriteEnabled = WGPUOptionalBool_False;
            depthStencilState.depthCompare = WGPUCompareFunction_LessEqual;
            depthStencilState.stencilReadMask = 0;
            depthStencilState.stencilWriteMask = 0;

            // Create render pipeline descriptor
            WGPURenderPipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor.label = init::makeStringView("Selection Render Pipeline");

            // Vertex state
            pipelineDescriptor.vertex.module = m_vertexShader;
            pipelineDescriptor.vertex.entryPoint = init::makeStringView("vs_main");
            pipelineDescriptor.vertex.bufferCount = 1;
            pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

            // Fragment state
            WGPUFragmentState fragmentState = {};
            fragmentState.module = m_fragmentShader;
            fragmentState.entryPoint = init::makeStringView("fs_main");

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

            // Depth stencil
            pipelineDescriptor.depthStencil = &depthStencilState;

            // Multisample
            pipelineDescriptor.multisample.count = 1;
            pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
            pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

            pipelineDescriptor.layout = pipelineLayout;

            m_renderPipeline.pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

            wgpuPipelineLayoutRelease(pipelineLayout);
        }

        // Create Bind Group
        {
            std::vector<WGPUBindGroupEntry> bindings;

            WGPUBindGroupEntry cameraBinding = {};
            cameraBinding.binding = 0;
            cameraBinding.buffer = m_cameraUniformBuffer;
            cameraBinding.offset = 0;
            cameraBinding.size = sizeof(CameraUniform);
            bindings.push_back(cameraBinding);

            WGPUBindGroupEntry modelBinding = {};
            modelBinding.binding = 1;
            modelBinding.buffer = m_modelUniformBuffer;
            modelBinding.offset = 0;
            modelBinding.size = sizeof(ModelUniform);
            bindings.push_back(modelBinding);

            WGPUBindGroupDescriptor bindGroupDesc = {};
            bindGroupDesc.layout = m_renderPipeline.bindGroupLayout;
            bindGroupDesc.entryCount = bindings.size();
            bindGroupDesc.entries = bindings.data();
            m_renderPipeline.bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
        }

        std::cout << "Selection renderer initialized." << std::endl;
    }

    void SelectionRenderer::create_mesh(WGPUDevice device)
    {
        m_selectionMesh.generate(device);
    }

    void SelectionRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera, const std::optional<glm::ivec3> &selected_block_pos)
    {
        is_visible = selected_block_pos.has_value();

        if (!is_visible)
        {
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
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.bindGroup, 0, nullptr);

        // Draw the selection mesh
        m_selectionMesh.render(renderPass);
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