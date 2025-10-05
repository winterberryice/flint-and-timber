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
        WGPUBindGroupLayout *pBindGroupLayout,
        bool useTexture,
    bool useModelMatrix,
        bool depthWriteEnabled,
        WGPUCompareFunction depthCompare)
    {
        std::cout << "Creating render pipeline..." << std::endl;

        // Depth Stencil State
        WGPUDepthStencilState depthStencilState = {};
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencilState.depthCompare = depthCompare;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        // Create bind group layout
        std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries;

    // Binding 0: Camera Uniform Buffer (Vertex) - Always present
    WGPUBindGroupLayoutEntry cameraUniformEntry = {};
    cameraUniformEntry.binding = 0;
    cameraUniformEntry.visibility = WGPUShaderStage_Vertex;
    cameraUniformEntry.buffer.type = WGPUBufferBindingType_Uniform;
    cameraUniformEntry.buffer.minBindingSize = sizeof(glm::mat4); // Minimum size, can be larger
    bindingLayoutEntries.push_back(cameraUniformEntry);

    int nextBinding = 1;

    if (useModelMatrix)
    {
        WGPUBindGroupLayoutEntry modelUniformEntry = {};
        modelUniformEntry.binding = nextBinding++;
        modelUniformEntry.visibility = WGPUShaderStage_Vertex;
        modelUniformEntry.buffer.type = WGPUBufferBindingType_Uniform;
        modelUniformEntry.buffer.minBindingSize = sizeof(glm::mat4);
        bindingLayoutEntries.push_back(modelUniformEntry);
    }

        if (useTexture)
        {
        // Binding 1 or 2: Texture View (Fragment)
            WGPUBindGroupLayoutEntry textureEntry = {};
        textureEntry.binding = nextBinding++;
            textureEntry.visibility = WGPUShaderStage_Fragment;
            textureEntry.texture.sampleType = WGPUTextureSampleType_Float;
            textureEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
            bindingLayoutEntries.push_back(textureEntry);

        // Binding 2 or 3: Sampler (Fragment)
            WGPUBindGroupLayoutEntry samplerEntry = {};
        samplerEntry.binding = nextBinding++;
            samplerEntry.visibility = WGPUShaderStage_Fragment;
            samplerEntry.sampler.type = WGPUSamplerBindingType_Filtering;
            bindingLayoutEntries.push_back(samplerEntry);
        }

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
        pipelineDescriptor.label = makeStringView("Render Pipeline");

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

        // Enable blending for transparency
        WGPUBlendState blendState = {};
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
        blendState.alpha.operation = WGPUBlendOperation_Add;
        colorTarget.blend = &blendState;

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive state
        pipelineDescriptor.primitive.topology = topology;
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
    WGPUBuffer cameraUniformBuffer,
    WGPUBuffer modelUniformBuffer,
        WGPUTextureView textureView,
        WGPUSampler sampler,
    bool useTexture,
    bool useModelMatrix)
    {
        std::vector<WGPUBindGroupEntry> bindings;
    int nextBinding = 0;

    // Binding 0: Camera Uniform Buffer - Always present
    WGPUBindGroupEntry cameraUniformBinding = {};
    cameraUniformBinding.binding = nextBinding++;
    cameraUniformBinding.buffer = cameraUniformBuffer;
    cameraUniformBinding.offset = 0;
    cameraUniformBinding.size = sizeof(glm::mat4); // CameraUniform size
    bindings.push_back(cameraUniformBinding);

    if (useModelMatrix)
    {
        WGPUBindGroupEntry modelUniformBinding = {};
        modelUniformBinding.binding = nextBinding++;
        modelUniformBinding.buffer = modelUniformBuffer;
        modelUniformBinding.offset = 0;
        modelUniformBinding.size = sizeof(glm::mat4); // ModelUniform size
        bindings.push_back(modelUniformBinding);
    }

        if (useTexture)
        {
        // Bindings for texture and sampler
            WGPUBindGroupEntry textureBinding = {};
        textureBinding.binding = nextBinding++;
            textureBinding.textureView = textureView;
            bindings.push_back(textureBinding);

            WGPUBindGroupEntry samplerBinding = {};
        samplerBinding.binding = nextBinding++;
            samplerBinding.sampler = sampler;
            bindings.push_back(samplerBinding);
        }

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