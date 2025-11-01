#include "text_renderer.h"

#include <iostream>
#include <fstream>
#include <vector>

#include "../init/shader.h"
#include "../init/utils.h"
#include "text_shader.wgsl.h"

namespace flint::graphics
{

    TextRenderer::TextRenderer() = default;

    TextRenderer::~TextRenderer()
    {
        delete[] m_cdata;
    }

    void TextRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, int width, int height)
    {
        std::cout << "Initializing text renderer..." << std::endl;
        m_windowWidth = width;
        m_windowHeight = height;

        // 1. Load Font File
        std::ifstream fontFile("assets/fonts/Roboto-Regular.ttf", std::ios::binary | std::ios::ate);
        if (!fontFile.is_open()) {
            std::cerr << "Error: Could not open font file." << std::endl;
            return;
        }
        std::streamsize size = fontFile.tellg();
        fontFile.seekg(0, std::ios::beg);
        std::vector<char> fontBuffer(size);
        if (!fontFile.read(fontBuffer.data(), size)) {
             std::cerr << "Error: Could not read font file." << std::endl;
            return;
        }
        fontFile.close();

        // 2. Bake Font Bitmap
        const int bitmapWidth = 512;
        const int bitmapHeight = 512;
        const float fontSize = 32.0f;
        std::vector<uint8_t> fontBitmap(bitmapWidth * bitmapHeight);
        m_cdata = new stbtt_bakedchar[96]; // ASCII 32..126 is 95 glyphs

        stbtt_BakeFontBitmap(
            reinterpret_cast<const unsigned char*>(fontBuffer.data()),
            0,          // font offset
            fontSize,      // font height in pixels
            fontBitmap.data(), // output bitmap
            bitmapWidth,    // bitmap width
            bitmapHeight,   // bitmap height
            32,         // first char to bake
            96,         // number of chars to bake
            m_cdata       // output baked char data
        );

        // 3. Create WebGPU Texture
        WGPUTextureDescriptor textureDesc = {};
        textureDesc.label = "Font Texture";
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = { (uint32_t)bitmapWidth, (uint32_t)bitmapHeight, 1 };
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.format = WGPUTextureFormat_R8Unorm;
        textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        m_fontTexture = wgpuDeviceCreateTexture(device, &textureDesc);

        // 4. Upload Texture Data
        WGPUImageCopyTexture destination = {};
        destination.texture = m_fontTexture;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 };

        WGPUTextureDataLayout source = {};
        source.offset = 0;
        source.bytesPerRow = bitmapWidth;
        source.rowsPerImage = bitmapHeight;

        wgpuQueueWriteTexture(queue, &destination, fontBitmap.data(), fontBitmap.size(), &source, &textureDesc.size);

        // 5. Create Texture View
        WGPUTextureViewDescriptor textureViewDesc = {};
        textureViewDesc.label = "Font Texture View";
        textureViewDesc.format = textureDesc.format;
        textureViewDesc.dimension = WGPUTextureViewDimension_2D;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.aspect = WGPUTextureAspect_All;
        m_fontTextureView = wgpuTextureCreateView(m_fontTexture, &textureViewDesc);

        // 6. Create Sampler
        WGPUSamplerDescriptor samplerDesc = {};
        samplerDesc.label = "Font Sampler";
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
        m_fontSampler = wgpuDeviceCreateSampler(device, &samplerDesc);

        // Create shaders
        m_vertexShader = init::create_shader_module(device, "Text Vertex Shader", TEXT_WGSL_shaderSource.data());
        m_fragmentShader = init::create_shader_module(device, "Text Fragment Shader", TEXT_WGSL_shaderSource.data());

        // Create Render Pipeline
        {
            // Bind group layout
            std::vector<WGPUBindGroupLayoutEntry> bindGroupLayoutEntries(2);
            // Texture view
            bindGroupLayoutEntries[0].binding = 0;
            bindGroupLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
            bindGroupLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
            bindGroupLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
            // Sampler
            bindGroupLayoutEntries[1].binding = 1;
            bindGroupLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
            bindGroupLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

            WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
            bindGroupLayoutDesc.label = "Text Bind Group Layout";
            bindGroupLayoutDesc.entryCount = bindGroupLayoutEntries.size();
            bindGroupLayoutDesc.entries = bindGroupLayoutEntries.data();
            m_renderPipeline.bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

            // Create pipeline layout
            WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
            pipelineLayoutDesc.bindGroupLayoutCount = 1;
            pipelineLayoutDesc.bindGroupLayouts = &m_renderPipeline.bindGroupLayout;
            WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

            // Vertex layout
            WGPUVertexBufferLayout vertexBufferLayout = flint::Vertex::getLayout();

            // Create render pipeline descriptor
            WGPURenderPipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor.label = "Text Render Pipeline";

            // Vertex state
            pipelineDescriptor.vertex.module = m_vertexShader;
            pipelineDescriptor.vertex.entryPoint = "vs_main";
            pipelineDescriptor.vertex.bufferCount = 1;
            pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

            // Fragment state
            WGPUFragmentState fragmentState = {};
            fragmentState.module = m_fragmentShader;
            fragmentState.entryPoint = "fs_main";

            // Color target
            WGPUColorTargetState colorTarget = {};
            colorTarget.format = surfaceFormat;
            colorTarget.writeMask = WGPUColorWriteMask_All;

            // Enable blending
            WGPUBlendState blendState = {};
            blendState.color.operation = WGPUBlendOperation_Add;
            blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
            blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            blendState.alpha.operation = WGPUBlendOperation_Add;
            blendState.alpha.srcFactor = WGPUBlendFactor_One;
            blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            colorTarget.blend = &blendState;

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

        // Create bind group
        std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
        // Texture view
        bindGroupEntries[0].binding = 0;
        bindGroupEntries[0].textureView = m_fontTextureView;
        // Sampler
        bindGroupEntries[1].binding = 1;
        bindGroupEntries[1].sampler = m_fontSampler;

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.label = "Text Bind Group";
        bindGroupDesc.layout = m_renderPipeline.bindGroupLayout;
        bindGroupDesc.entryCount = bindGroupEntries.size();
        bindGroupDesc.entries = bindGroupEntries.data();
        m_renderPipeline.bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        rebuild_mesh(device, queue);

        std::cout << "Text renderer initialized." << std::endl;
    }

    void TextRenderer::render(WGPURenderPassEncoder renderPass)
    {
        wgpuRenderPassEncoderSetPipeline(renderPass, m_renderPipeline.pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_renderPipeline.bindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
    }

    void TextRenderer::onResize(WGPUDevice device, WGPUQueue queue, int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;
        rebuild_mesh(device, queue);
    }

    void TextRenderer::cleanup()
    {
        std::cout << "Cleaning up text renderer..." << std::endl;

        m_renderPipeline.cleanup();

        if (m_fontTextureView) {
            wgpuTextureViewRelease(m_fontTextureView);
            m_fontTextureView = nullptr;
        }
        if (m_fontTexture) {
            wgpuTextureRelease(m_fontTexture);
            m_fontTexture = nullptr;
        }
        if (m_fontSampler) {
            wgpuSamplerRelease(m_fontSampler);
            m_fontSampler = nullptr;
        }
        if (m_vertexBuffer)
        {
            wgpuBufferDestroy(m_vertexBuffer);
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer)
        {
            wgpuBufferDestroy(m_indexBuffer);
            wgpuBufferRelease(m_indexBuffer);
            m_indexBuffer = nullptr;
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

    void TextRenderer::rebuild_mesh(WGPUDevice device, WGPUQueue queue)
    {
        if (m_vertexBuffer)
        {
            wgpuBufferDestroy(m_vertexBuffer);
            wgpuBufferRelease(m_vertexBuffer);
        }
        if (m_indexBuffer)
        {
            wgpuBufferDestroy(m_indexBuffer);
            wgpuBufferRelease(m_indexBuffer);
        }

        TextMeshData meshData = create_text_mesh("hello world");

        // Create vertex buffer
        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.label = "Text Vertex Buffer";
        vertexBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
        vertexBufferDesc.size = meshData.vertices.size() * sizeof(flint::Vertex);
        vertexBufferDesc.mappedAtCreation = false;
        m_vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
        wgpuQueueWriteBuffer(queue, m_vertexBuffer, 0, meshData.vertices.data(), vertexBufferDesc.size);

        // Create index buffer
        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.label = "Text Index Buffer";
        indexBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        indexBufferDesc.size = meshData.indices.size() * sizeof(uint32_t);
        indexBufferDesc.mappedAtCreation = false;
        m_indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);
        wgpuQueueWriteBuffer(queue, m_indexBuffer, 0, meshData.indices.data(), indexBufferDesc.size);
        m_indexCount = meshData.indices.size();
    }

    TextMeshData TextRenderer::create_text_mesh(const std::string& text)
    {
        TextMeshData meshData;

        float x = 10.0f; // Start at pixel 10
        float y = 32.0f; // Start at pixel 32 (font size)

        for (char c : text)
        {
            if (c >= 32 && c < 128)
            {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(m_cdata, 512, 512, c - 32, &x, &y, &q, 1);

                float x0 = (q.x0 / m_windowWidth) * 2.0f - 1.0f;
                float y0 = (q.y0 / m_windowHeight) * 2.0f - 1.0f;
                float x1 = (q.x1 / m_windowWidth) * 2.0f - 1.0f;
                float y1 = (q.y1 / m_windowHeight) * 2.0f - 1.0f;

                uint32_t baseIndex = meshData.vertices.size();

                meshData.vertices.push_back({ {x0, -y0, 0.0f}, {1.0f, 1.0f, 1.0f}, {q.s0, q.t0}, 1.0f });
                meshData.vertices.push_back({ {x1, -y0, 0.0f}, {1.0f, 1.0f, 1.0f}, {q.s1, q.t0}, 1.0f });
                meshData.vertices.push_back({ {x1, -y1, 0.0f}, {1.0f, 1.0f, 1.0f}, {q.s1, q.t1}, 1.0f });
                meshData.vertices.push_back({ {x0, -y1, 0.0f}, {1.0f, 1.0f, 1.0f}, {q.s0, q.t1}, 1.0f });

                meshData.indices.push_back(baseIndex);
                meshData.indices.push_back(baseIndex + 1);
                meshData.indices.push_back(baseIndex + 2);
                meshData.indices.push_back(baseIndex);
                meshData.indices.push_back(baseIndex + 2);
                meshData.indices.push_back(baseIndex + 3);
            }
        }

        return meshData;
    }

} // namespace flint::graphics