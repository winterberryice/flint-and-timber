#include "pipeline.h"
#include "../vertex.h" // For Vertex::getLayout()
#include "utils.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace flint::init
{
    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        const std::vector<WGPUBindGroupLayout> &bindGroupLayouts)
    {
        std::cout << "Creating render pipeline..." << std::endl;

        // Depth Stencil State
        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
        depthStencilState.depthCompare = WGPUCompareFunction_Less;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        // Create pipeline layout using the provided bind group layouts
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
        pipelineLayoutDesc.bindGroupLayouts = bindGroupLayouts.data();

        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);
        if (!pipelineLayout)
        {
            std::cerr << "Failed to create pipeline layout!" << std::endl;
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Get the vertex buffer layout from the Vertex struct itself.
        WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

        // Create render pipeline descriptor
        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Flint Render Pipeline");

        // Vertex state
        pipelineDescriptor.vertex.module = vertexShader;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        // Fragment state
        WGPUFragmentState fragmentState = {};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");

        // Color target (what we're rendering to)
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive state
        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;

        // Multisample state
        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        // Set depth stencil state
        pipelineDescriptor.depthStencil = &depthStencilState;

        pipelineDescriptor.layout = pipelineLayout;

        WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

        wgpuPipelineLayoutRelease(pipelineLayout);

        if (!renderPipeline)
        {
            std::cerr << "Failed to create render pipeline!" << std::endl;
            throw std::runtime_error("Failed to create render pipeline!");
        }

        std::cout << "Render pipeline created successfully" << std::endl;
        return renderPipeline;
    }
} // namespace flint::init