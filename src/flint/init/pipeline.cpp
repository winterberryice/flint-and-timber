#include "pipeline.h"

#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "../vertex.h"
#include "../camera.h"
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
        bool useModel,
        bool depthWriteEnabled,
        WGPUCompareFunction depthCompare,
        bool useBlending,
    bool useCulling,
    WGPUPrimitiveTopology primitiveTopology,
    bool isUi)
    {
        std::cout << "Creating render pipeline..." << std::endl;

        // Depth Stencil State
        WGPUDepthStencilState depthStencilState = {};
    if (depthTextureFormat != WGPUTextureFormat_Undefined)
    {
        depthStencilState.format = depthTextureFormat;
        depthStencilState.depthWriteEnabled = depthWriteEnabled;
        depthStencilState.depthCompare = depthCompare;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;
    }

        // Create bind group layout
        std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries;

    if (!isUi)
        {
        // Binding 0: Uniform Buffer (Vertex) - Always present
        WGPUBindGroupLayoutEntry uniformEntry = {};
        uniformEntry.binding = 0;
        uniformEntry.visibility = WGPUShaderStage_Vertex;
        uniformEntry.buffer.type = WGPUBufferBindingType_Uniform;
        uniformEntry.buffer.minBindingSize = sizeof(CameraUniform);
        bindingLayoutEntries.push_back(uniformEntry);

        if (useModel)
        {
            // Binding 1: Uniform Buffer (Vertex) for Model Matrix
            WGPUBindGroupLayoutEntry modelEntry = {};
            modelEntry.binding = 1;
            modelEntry.visibility = WGPUShaderStage_Vertex;
            modelEntry.buffer.type = WGPUBufferBindingType_Uniform;
            modelEntry.buffer.minBindingSize = sizeof(glm::mat4);
            bindingLayoutEntries.push_back(modelEntry);
        }

        uint32_t current_binding = useModel ? 2 : 1;

        if (useTexture)
        {
            // Binding 1 or 2: Texture View (Fragment)
            WGPUBindGroupLayoutEntry textureEntry = {};
            textureEntry.binding = current_binding++;
            textureEntry.visibility = WGPUShaderStage_Fragment;
            textureEntry.texture.sampleType = WGPUTextureSampleType_Float;
            textureEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
            bindingLayoutEntries.push_back(textureEntry);

            // Binding 2 or 3: Sampler (Fragment)
            WGPUBindGroupLayoutEntry samplerEntry = {};
            samplerEntry.binding = current_binding++;
            samplerEntry.visibility = WGPUShaderStage_Fragment;
            samplerEntry.sampler.type = WGPUSamplerBindingType_Filtering;
            bindingLayoutEntries.push_back(samplerEntry);
        }
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

    // Get vertex layout
    WGPUVertexBufferLayout vertexBufferLayout;
    std::vector<WGPUVertexAttribute> vertexAttributes;

    if (isUi)
    {
        vertexAttributes.resize(1);

        // position
        vertexAttributes[0].format = WGPUVertexFormat_Float32x2;
        vertexAttributes[0].offset = 0;
        vertexAttributes[0].shaderLocation = 0;

        vertexBufferLayout.arrayStride = 2 * sizeof(float);
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout.attributeCount = vertexAttributes.size();
        vertexBufferLayout.attributes = vertexAttributes.data();
    }
    else
    {
        vertexBufferLayout = flint::Vertex::getLayout();
    }

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

        WGPUBlendState blendState = {};
        if (useBlending)
        {
            blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
            blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            blendState.color.operation = WGPUBlendOperation_Add;
            blendState.alpha.srcFactor = WGPUBlendFactor_One;
            blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
            blendState.alpha.operation = WGPUBlendOperation_Add;
            colorTarget.blend = &blendState;
        }

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDescriptor.fragment = &fragmentState;

        // Primitive state
    pipelineDescriptor.primitive.topology = primitiveTopology;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = useCulling ? WGPUCullMode_Back : WGPUCullMode_None;

        // Multisample state
        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        // Set depth stencil state
    if (depthTextureFormat != WGPUTextureFormat_Undefined)
    {
        pipelineDescriptor.depthStencil = &depthStencilState;
    }
    else
    {
        pipelineDescriptor.depthStencil = nullptr;
    }

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
        bool useModel)
    {
        std::vector<WGPUBindGroupEntry> bindings;

        // Binding 0: Camera Uniform Buffer - Always present
        WGPUBindGroupEntry cameraBinding = {};
        cameraBinding.binding = 0;
        cameraBinding.buffer = cameraUniformBuffer;
        cameraBinding.offset = 0;
        cameraBinding.size = sizeof(CameraUniform);
        bindings.push_back(cameraBinding);

        uint32_t current_binding = 1;

        if (useModel)
        {
            WGPUBindGroupEntry modelBinding = {};
            modelBinding.binding = current_binding++;
            modelBinding.buffer = modelUniformBuffer;
            modelBinding.offset = 0;
            modelBinding.size = sizeof(glm::mat4);
            bindings.push_back(modelBinding);
        }

        if (useTexture)
        {
            // Binding 1 or 2: Texture View
            WGPUBindGroupEntry textureBinding = {};
            textureBinding.binding = current_binding++;
            textureBinding.textureView = textureView;
            bindings.push_back(textureBinding);

            // Binding 2 or 3: Sampler
            WGPUBindGroupEntry samplerBinding = {};
            samplerBinding.binding = current_binding++;
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