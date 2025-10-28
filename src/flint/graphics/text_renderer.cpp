#include "text_renderer.h"

#include "../text_shader.wgsl.h"
#include "../init/shader.h"
#include "../init/pipeline.h"
#include "../init/texture.h"
#include "../init/utils.h"

#include "stb_truetype.h"

#include <iostream>
#include <vector>
#include <fstream>

namespace flint::graphics
{
    void TextRenderer::init(WGPUDevice device, WGPUTextureFormat surfaceFormat)
    {
        create_font_texture(device);
        create_vertex_buffer(device);

        WGPUShaderModule shaderModule = init::create_shader_module(device, "Text Shader", shaders::text_shader_source.c_str());

        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.nextInChain = nullptr;

        WGPUBindGroupLayoutEntry bindGroupLayoutEntries[2] = {};
        bindGroupLayoutEntries[0].binding = 0;
        bindGroupLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
        bindGroupLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
        bindGroupLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        bindGroupLayoutEntries[1].binding = 1;
        bindGroupLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
        bindGroupLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = 2;
        bindGroupLayoutDesc.entries = bindGroupLayoutEntries;
        WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        pipelineDesc.layout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        // Vertex state
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.bufferCount = 1;
        WGPUVertexBufferLayout vertexBufferLayout = {};
        vertexBufferLayout.arrayStride = 4 * sizeof(float); // 2 for pos, 2 for uv
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout.attributeCount = 2;
        WGPUVertexAttribute attributes[2] = {};
        attributes[0].format = WGPUVertexFormat_Float32x2;
        attributes[0].offset = 0;
        attributes[0].shaderLocation = 0;
        attributes[1].format = WGPUVertexFormat_Float32x2;
        attributes[1].offset = 2 * sizeof(float);
        attributes[1].shaderLocation = 1;
        vertexBufferLayout.attributes = attributes;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        // Fragment state
        WGPUFragmentState fragmentState = {};
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.targetCount = 1;
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = surfaceFormat;
        WGPUBlendState blendState = {
            .color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
            .alpha = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            }
        };
        colorTarget.blend = &blendState;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        fragmentState.targets = &colorTarget;
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDesc.primitive.cullMode = WGPUCullMode_None;

        pipelineDesc.depthStencil = nullptr;

        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = 0xFFFFFFFF;
        pipelineDesc.multisample.alphaToCoverageEnabled = false;

        m_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
        wgpuShaderModuleRelease(shaderModule);

        // Create bind group
        WGPUBindGroupEntry bindGroupEntries[2] = {};
        bindGroupEntries[0].binding = 0;
        bindGroupEntries[0].textureView = m_fontTextureView;

        bindGroupEntries[1].binding = 1;
        bindGroupEntries[1].sampler = m_sampler;

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 2;
        bindGroupDesc.entries = bindGroupEntries;
        m_bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

    }

    void TextRenderer::render(WGPURenderPassEncoder renderPass)
    {
        wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);
    }

    void TextRenderer::cleanup()
    {
        wgpuRenderPipelineRelease(m_pipeline);
        wgpuBufferRelease(m_vertexBuffer);
        wgpuTextureViewRelease(m_fontTextureView);
        wgpuTextureRelease(m_fontTexture);
        wgpuSamplerRelease(m_sampler);
        wgpuBindGroupRelease(m_bindGroup);
    }

    void TextRenderer::create_font_texture(WGPUDevice device)
    {
        std::ifstream file("assets/fonts/Roboto-Regular.ttf", std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size))
        {
            throw std::runtime_error("Failed to read font file");
        }

        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, (const unsigned char*)buffer.data(), 0))
        {
            throw std::runtime_error("Failed to initialize font");
        }

        m_textureWidth = 512;
        m_textureHeight = 512;
        std::vector<unsigned char> bitmap(m_textureWidth * m_textureHeight, 0);

        float scale = stbtt_ScaleForPixelHeight(&font, 32);
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

        int x = 10;
        int y = 10 + ascent * scale;

        const char* text = "hello world";
        for (int i = 0; text[i]; ++i)
        {
            int advance_width;
            stbtt_GetCodepointHMetrics(&font, text[i], &advance_width, 0);

            int glyph = stbtt_FindGlyphIndex(&font, text[i]);

            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&font, glyph, scale, scale, &x0, &y0, &x1, &y1);

            int byteOffset = x + (y + y0) * m_textureWidth;
            stbtt_MakeGlyphBitmap(&font, bitmap.data() + byteOffset, x1 - x0, y1 - y0, m_textureWidth, scale, scale, glyph);

            x += advance_width * scale;
            if (text[i+1])
            {
                int kern = stbtt_GetCodepointKernAdvance(&font, text[i], text[i+1]);
                x += kern * scale;
            }

        }

        WGPUTextureDescriptor textureDesc = {};
        textureDesc.size = { (uint32_t)m_textureWidth, (uint32_t)m_textureHeight, 1 };
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.format = WGPUTextureFormat_R8Unorm;
        textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        m_fontTexture = wgpuDeviceCreateTexture(device, &textureDesc);

        WGPUTextureViewDescriptor textureViewDesc = {};
        textureViewDesc.format = WGPUTextureFormat_R8Unorm;
        textureViewDesc.dimension = WGPUTextureViewDimension_2D;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        m_fontTextureView = wgpuTextureCreateView(m_fontTexture, &textureViewDesc);

        WGPUSamplerDescriptor samplerDesc = {};
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.minFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        samplerDesc.lodMinClamp = 0.0f;
        samplerDesc.lodMaxClamp = 1.0f;
        samplerDesc.compare = WGPUCompareFunction_Undefined;
        samplerDesc.maxAnisotropy = 1;
        m_sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

        WGPUQueue queue = wgpuDeviceGetQueue(device);
        WGPUImageCopyTexture imageCopyTexture = {};
        imageCopyTexture.texture = m_fontTexture;
        imageCopyTexture.mipLevel = 0;
        imageCopyTexture.origin = { 0, 0, 0 };
        WGPUTextureDataLayout textureDataLayout = {};
        textureDataLayout.offset = 0;
        textureDataLayout.bytesPerRow = m_textureWidth;
        textureDataLayout.rowsPerImage = m_textureHeight;
        WGPUExtent3D writeSize = { (uint32_t)m_textureWidth, (uint32_t)m_textureHeight, 1 };
        wgpuQueueWriteTexture(queue, &imageCopyTexture, bitmap.data(), bitmap.size(), &textureDataLayout, &writeSize);
    }

    void TextRenderer::create_vertex_buffer(WGPUDevice device)
    {
        float x = -0.9f;
        float y = 0.9f;
        float w = 0.5f;
        float h = 0.5f;

        std::vector<float> vertices = {
            x,     y,     0.0f, 0.0f,
            x + w, y,     1.0f, 0.0f,
            x,     y - h, 0.0f, 1.0f,
            x,     y - h, 0.0f, 1.0f,
            x + w, y,     1.0f, 0.0f,
            x + w, y - h, 1.0f, 1.0f,
        };

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size = vertices.size() * sizeof(float);
        bufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bufferDesc.mappedAtCreation = false;
        m_vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        WGPUQueue queue = wgpuDeviceGetQueue(device);
        wgpuQueueWriteBuffer(queue, m_vertexBuffer, 0, vertices.data(), bufferDesc.size);
    }
}
