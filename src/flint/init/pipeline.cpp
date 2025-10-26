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
        renderPassDesc.label = "Render Pass";
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

        return wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    }

    WGPURenderPassEncoder begin_overlay_render_pass(
        WGPUCommandEncoder encoder,
        WGPUTextureView textureView)
    {
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = textureView;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.loadOp = WGPULoadOp_Load; // Load the existing content
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f}; // Not used
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain = nullptr;
        renderPassDesc.label = "Overlay Render Pass";
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr; // No depth/stencil

        return wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    }

    WGPURenderPipeline create_render_pipeline(WGPUDevice device, WGPUShaderModule shaderModule, WGPUFragmentState *fragmentState, WGPUVertexBufferLayout *vertexBufferLayout, WGPUPrimitiveTopology primitiveTopology, WGPUDepthStencilState *depthStencilState)
    {
        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.nextInChain = nullptr;
        pipelineDesc.label = "Render Pipeline";

        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = vertexBufferLayout;

        pipelineDesc.primitive.topology = primitiveTopology;
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDesc.primitive.cullMode = WGPUCullMode_Back;

        pipelineDesc.fragment = fragmentState;

        pipelineDesc.depthStencil = depthStencilState;

        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = 0xFFFFFFFF;
        pipelineDesc.multisample.alphaToCoverageEnabled = false;

        return wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    }

    WGPUBindGroup create_bind_group(WGPUDevice device, WGPURenderPipeline pipeline, uint32_t groupIndex, WGPUBindGroupEntry *entries, uint32_t entryCount)
    {
        WGPUBindGroupLayout bindGroupLayout = wgpuRenderPipelineGetBindGroupLayout(pipeline, groupIndex);

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.label = "Bind Group";
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = entryCount;
        bindGroupDesc.entries = entries;

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
        wgpuBindGroupLayoutRelease(bindGroupLayout);
        return bindGroup;
    }

} // namespace flint::init
