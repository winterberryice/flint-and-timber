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
        WGPUTextureFormat depthTextureFormat, // New parameter
        WGPUBindGroupLayout *pBindGroupLayout)
    {
        std::cout << "Creating render pipeline..." << std::endl;

        // Depth Stencil State
        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
        depthStencilState.depthCompare = WGPUCompareFunction_Less;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        // Create bind group layout for uniforms, texture, and sampler
        std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

        // Binding 0: Uniform Buffer (Vertex)
        bindingLayoutEntries[0].binding = 0;
        bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
        bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayoutEntries[0].buffer.minBindingSize = sizeof(glm::mat4);

        // Binding 1: Texture View (Fragment)
        bindingLayoutEntries[1].binding = 1;
        bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
        bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
        bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

        // Binding 2: Sampler (Fragment)
        bindingLayoutEntries[2].binding = 2;
        bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
        bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();
        bindGroupLayoutDesc.entries = bindingLayoutEntries.data();

        WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
        if (!bindGroupLayout)
        {
            std::cerr << "Failed to create bind group layout!" << std::endl;
            throw std::runtime_error("Failed to create bind group layout!");
        }
        *pBindGroupLayout = bindGroupLayout; // Output the layout

        // Create pipeline layout using our bind group layout
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;

        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        // Get vertex layout from the Vertex struct
        WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

        // Create render pipeline descriptor
        WGPURenderPipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor.label = makeStringView("Cube Render Pipeline");

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

    WGPUBindGroup create_bind_group(
        WGPUDevice device,
        WGPUBindGroupLayout bindGroupLayout,
        WGPUBuffer uniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler)
    {
        std::vector<WGPUBindGroupEntry> bindings(3);

        // Binding 0: Uniform Buffer
        bindings[0].binding = 0;
        bindings[0].buffer = uniformBuffer;
        bindings[0].offset = 0;
        bindings[0].size = sizeof(glm::mat4);

        // Binding 1: Texture View
        bindings[1].binding = 1;
        bindings[1].textureView = textureView;

        // Binding 2: Sampler
        bindings[2].binding = 2;
        bindings[2].sampler = sampler;

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
        if (!bindGroup)
        {
            std::cerr << "Failed to create bind group!" << std::endl;
            throw std::runtime_error("Failed to create bind group!");
        }

        return bindGroup;
    }

    WGPURenderPassEncoder begin_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView,
        WGPUTextureView depthTextureView)
    {
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = textureView;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f}; // Dark blue background
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
        depthStencilAttachment.view = depthTextureView;
        depthStencilAttachment.depthClearValue = 1.0f;
        depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthStencilAttachment.depthReadOnly = false;
        depthStencilAttachment.stencilClearValue = 0;
        depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
        depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
        depthStencilAttachment.stencilReadOnly = true;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain = nullptr;
        renderPassDesc.label = {nullptr, 0};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

        return wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    }

} // namespace flint::init