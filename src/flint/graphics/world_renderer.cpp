#include "world_renderer.h"

#include <iostream>
#include <vector>
#include <stdexcept>

#include "atlas_bytes.hpp"
#include "../init/buffer.h"
#include "../init/shader.h"
#include "../init/utils.h"
#include "../shader.wgsl.h"
#include "../vertex.h"
#include "../camera.h"

namespace flint::graphics
{

    WorldRenderer::WorldRenderer() = default;

    WorldRenderer::~WorldRenderer() = default;

    void WorldRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat)
    {
        std::cout << "Initializing world renderer..." << std::endl;

        // Load texture atlas
        if (!m_atlas.loadFromMemory(device, queue, assets_textures_block_atlas_png, assets_textures_block_atlas_png_len))
        {
            std::cerr << "Failed to load texture atlas for world renderer!" << std::endl;
        }

        // Create shaders
        m_vertexShader = init::create_shader_module(device, "World Vertex Shader", WGSL_vertexShaderSource);
        m_fragmentShader = init::create_shader_module(device, "World Fragment Shader", WGSL_fragmentShaderSource);

        // Create uniform buffer for camera matrices
        m_uniformBuffer = init::create_uniform_buffer(device, "Camera Uniform Buffer", sizeof(CameraUniform));

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

            // Binding 1: Texture View (Fragment)
            WGPUBindGroupLayoutEntry textureEntry = {};
            textureEntry.binding = 1;
            textureEntry.visibility = WGPUShaderStage_Fragment;
            textureEntry.texture.sampleType = WGPUTextureSampleType_Float;
            textureEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
            bindingLayoutEntries.push_back(textureEntry);

            // Binding 2: Sampler (Fragment)
            WGPUBindGroupLayoutEntry samplerEntry = {};
            samplerEntry.binding = 2;
            samplerEntry.visibility = WGPUShaderStage_Fragment;
            samplerEntry.sampler.type = WGPUSamplerBindingType_Filtering;
            bindingLayoutEntries.push_back(samplerEntry);

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
            depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
            depthStencilState.depthCompare = WGPUCompareFunction_Less;
            depthStencilState.stencilReadMask = 0;
            depthStencilState.stencilWriteMask = 0;

            // Create render pipeline descriptor
            WGPURenderPipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor.label = init::makeStringView("World Render Pipeline");

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
            pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
            pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
            pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
            pipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;

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
            cameraBinding.buffer = m_uniformBuffer;
            cameraBinding.offset = 0;
            cameraBinding.size = sizeof(CameraUniform);
            bindings.push_back(cameraBinding);

            WGPUBindGroupEntry textureBinding = {};
            textureBinding.binding = 1;
            textureBinding.textureView = m_atlas.getView();
            bindings.push_back(textureBinding);

            WGPUBindGroupEntry samplerBinding = {};
            samplerBinding.binding = 2;
            samplerBinding.sampler = m_atlas.getSampler();
            bindings.push_back(samplerBinding);

            WGPUBindGroupDescriptor bindGroupDesc = {};
            bindGroupDesc.layout = m_renderPipeline.bindGroupLayout;
            bindGroupDesc.entryCount = bindings.size();
            bindGroupDesc.entries = bindings.data();
            m_renderPipeline.bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
        }

        std::cout << "World renderer initialized." << std::endl;

        rebuild_chunk_mesh(device);
    }

    void WorldRenderer::rebuild_chunk_mesh(WGPUDevice device)
    {
        m_chunkMesh.generate(device, *m_world.getChunk());
    }

    World &WorldRenderer::getWorld()
    {
        return m_world;
    }

    const World &WorldRenderer::getWorld() const
    {
        return m_world;
    }

    void WorldRenderer::render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera)
    {
        // Update the uniform buffer with the new camera view-projection matrix
        m_cameraUniform.updateViewProj(camera);
        wgpuQueueWriteBuffer(queue, m_uniformBuffer, 0, &m_cameraUniform, sizeof(CameraUniform));

        // Set pipeline and bind group
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.bindGroup, 0, nullptr);

        // Draw the chunk
        m_chunkMesh.render(renderPass);
    }

    void WorldRenderer::cleanup()
    {
        std::cout << "Cleaning up world renderer..." << std::endl;

        m_renderPipeline.cleanup();
        m_atlas.cleanup();
        m_chunkMesh.cleanup();

        if (m_uniformBuffer)
        {
            wgpuBufferRelease(m_uniformBuffer);
            m_uniformBuffer = nullptr;
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