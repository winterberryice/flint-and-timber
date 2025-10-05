#include "pipeline.h"

#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "../vertex.h"
#include "utils.h"

namespace flint::init
{

    WGPURenderPipeline create_render_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        WGPUBindGroupLayout *pBindGroupLayout)
    {
        std::cout << "Creating render pipeline..." << std::endl;

        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
        depthStencilState.depthCompare = WGPUCompareFunction_Less;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

        bindingLayoutEntries[0].binding = 0;
        bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
        bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayoutEntries[0].buffer.minBindingSize = sizeof(glm::mat4);

        bindingLayoutEntries[1].binding = 1;
        bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
        bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
        bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

        bindingLayoutEntries[2].binding = 2;
        bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
        bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();
        bindGroupLayoutDesc.entries = bindingLayoutEntries.data();

        WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
        if (!bindGroupLayout)
        {
            throw std::runtime_error("Failed to create bind group layout!");
        }
        *pBindGroupLayout = bindGroupLayout;

        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;

        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Cube Render Pipeline");

        pipelineDescriptor.vertex.module = vertexShader;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        WGPUFragmentState fragmentState = {};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");

        WGPUColorTargetState colorTarget = {};
        colorTarget.format = surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;

        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        pipelineDescriptor.depthStencil = &depthStencilState;
        pipelineDescriptor.layout = pipelineLayout;

        WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

        wgpuPipelineLayoutRelease(pipelineLayout);

        if (!renderPipeline)
        {
            throw std::runtime_error("Failed to create render pipeline!");
        }

        std::cout << "Render pipeline created successfully" << std::endl;
        return renderPipeline;
    }

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
        WGPUBuffer uniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler)
    {
        std::vector<WGPUBindGroupEntry> bindings(3);

        bindings[0].binding = 0;
        bindings[0].buffer = uniformBuffer;
        bindings[0].offset = 0;
        bindings[0].size = sizeof(glm::mat4);

        bindings[1].binding = 1;
        bindings[1].textureView = textureView;

        bindings[2].binding = 2;
        bindings[2].sampler = sampler;

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
        if (!bindGroup)
        {
            throw std::runtime_error("Failed to create bind group!");
        }

        return bindGroup;
    }

    WGPURenderPipeline create_selection_pipeline(
        WGPUDevice device,
        WGPUShaderModule vertexShader,
        WGPUShaderModule fragmentShader,
        WGPUTextureFormat surfaceFormat,
        WGPUTextureFormat depthTextureFormat,
        WGPUBindGroupLayout bindGroupLayout) // Takes the main layout
    {
        std::cout << "Creating selection pipeline..." << std::endl;

        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
        depthStencilState.depthCompare = WGPUCompareFunction_Less;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;
        depthStencilState.depthBias = -1;

        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout; // Use the shared layout
        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Selection Render Pipeline");
        pipelineDescriptor.layout = pipelineLayout;
        pipelineDescriptor.vertex.module = vertexShader;
        pipelineDescriptor.vertex.entryPoint = makeStringView("vs_main");
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        WGPUFragmentState fragmentState = {};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = makeStringView("fs_main");
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = surfaceFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.cullMode = WGPUCullMode_None;

        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        pipelineDescriptor.depthStencil = &depthStencilState;

        WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);
        wgpuPipelineLayoutRelease(pipelineLayout);

        if (!renderPipeline)
        {
            throw std::runtime_error("Failed to create selection render pipeline!");
        }

        std::cout << "Selection pipeline created successfully" << std::endl;
        return renderPipeline;
    }

} // namespace flint::init