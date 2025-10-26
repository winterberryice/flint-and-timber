#include "debug_overlay_renderer.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" // User-provided dependency.

#include "../init/buffer.h"
#include "../init/pipeline.h"
#include "../init/shader.h"
#include "../init/texture.h"
#include "text_shader.wgsl.h"

namespace flint::graphics {

    namespace {
        // Simple file reading utility.
        std::vector<uint8_t> readFile(const std::string& path) {
            std::ifstream file(path, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << path << std::endl;
                return {};
            }
            size_t fileSize = (size_t)file.tellg();
            std::vector<uint8_t> buffer(fileSize);
            file.seekg(0);
            file.read((char*)buffer.data(), fileSize);
            file.close();
            return buffer;
        }
    }

    DebugOverlayRenderer::DebugOverlayRenderer() : m_charData(96) {
    }

    void DebugOverlayRenderer::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, uint32_t width, uint32_t height) {
        m_device = device;
        m_queue = queue;
        m_width = width;
        m_height = height;

        createFontTexture();
        createPipeline(surfaceFormat);
        createBindGroup();
    }

    void DebugOverlayRenderer::cleanup() {
        if (m_vertexBuffer) wgpuBufferRelease(m_vertexBuffer);
        if (m_bindGroup) wgpuBindGroupRelease(m_bindGroup);
        if (m_fontSampler) wgpuSamplerRelease(m_fontSampler);
        if (m_fontTextureView) wgpuTextureViewRelease(m_fontTextureView);
        if (m_fontTexture) wgpuTextureRelease(m_fontTexture);
        if (m_pipeline) wgpuRenderPipelineRelease(m_pipeline);
    }

    void DebugOverlayRenderer::toggle() {
        m_isVisible = !m_isVisible;
    }

    void DebugOverlayRenderer::update(const glm::vec3& playerPosition) {
        m_playerPosition = playerPosition;
        std::stringstream ss;
        ss.precision(2);
        ss << std::fixed;
        ss << "x: " << playerPosition.x << " y: " << playerPosition.y << " z: " << playerPosition.z;
        m_textToRender = ss.str();
    }

    void DebugOverlayRenderer::onResize(uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;
    }

    void DebugOverlayRenderer::createFontTexture() {
        auto font_file_data = readFile("assets/fonts/Roboto-Regular.ttf");
        if (font_file_data.empty()) {
            throw std::runtime_error("Failed to load font file.");
        }

        std::vector<uint8_t> font_atlas_bitmap(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);

        stbtt_BakeFontBitmap(
            font_file_data.data(),
            0,
            32.0f, // Font size in pixels
            font_atlas_bitmap.data(),
            FONT_ATLAS_WIDTH,
            FONT_ATLAS_HEIGHT,
            32, // First character to bake
            96, // Number of characters to bake
            m_charData.data()
        );

        init::create_texture(
            m_device,
            m_queue,
            font_atlas_bitmap.data(),
            FONT_ATLAS_WIDTH,
            FONT_ATLAS_HEIGHT,
            WGPUTextureFormat_R8Unorm,
            &m_fontTexture,
            &m_fontTextureView
        );

        m_fontSampler = init::create_sampler(m_device);
    }

    void DebugOverlayRenderer::createPipeline(WGPUTextureFormat surfaceFormat) {
        WGPUShaderModule shaderModule = init::create_shader_module(m_device, kTextShader);

        WGPUBlendState blendState = {};
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.alpha.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_One;
        blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

        WGPUColorTargetState colorTargetState = {};
        colorTargetState.format = surfaceFormat;
        colorTargetState.blend = &blendState;
        colorTargetState.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragmentState = {};
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTargetState;

        // Two vec2 floats per vertex: position and UV.
        WGPUVertexAttribute attributes[2] = {};
        attributes[0].format = WGPUVertexFormat_Float32x2;
        attributes[0].offset = 0;
        attributes[0].shaderLocation = 0;
        attributes[1].format = WGPUVertexFormat_Float32x2;
        attributes[1].offset = 2 * sizeof(float);
        attributes[1].shaderLocation = 1;

        WGPUVertexBufferLayout vertexBufferLayout = {};
        vertexBufferLayout.arrayStride = 4 * sizeof(float);
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout.attributeCount = 2;
        vertexBufferLayout.attributes = attributes;

        m_pipeline = init::create_render_pipeline(
            m_device,
            shaderModule,
            &fragmentState,
            &vertexBufferLayout,
            WGPUPrimitiveTopology_TriangleList,
            nullptr // No depth stencil
        );

        wgpuShaderModuleRelease(shaderModule);
    }

    void DebugOverlayRenderer::createBindGroup() {
        WGPUBindGroupEntry bgEntries[2] = {};
        bgEntries[0].binding = 0;
        bgEntries[0].textureView = m_fontTextureView;
        bgEntries[1].binding = 1;
        bgEntries[1].sampler = m_fontSampler;

        m_bindGroup = init::create_bind_group(m_device, m_pipeline, 0, bgEntries, 2);
    }

    void DebugOverlayRenderer::render(WGPURenderPassEncoder renderPass) {
        if (!m_isVisible || m_textToRender.empty()) {
            return;
        }

        // --- Generate vertex data for the text string ---
        std::vector<float> vertexData;
        vertexData.reserve(m_textToRender.length() * 6 * 4); // 6 vertices, 4 floats per vertex

        float x = 10.0f; // Start position x (in pixels)
        float y = 10.0f; // Start position y (in pixels)

        for (char c : m_textToRender) {
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(m_charData.data(), FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, c - 32, &x, &y, &q, 1);

                // Convert from screen pixel coordinates to clip space coordinates (-1 to 1)
                float x1 = (q.x0 / m_width) * 2.0f - 1.0f;
                float y1 = (q.y0 / m_height) * -2.0f + 1.0f;
                float x2 = (q.x1 / m_width) * 2.0f - 1.0f;
                float y2 = (q.y1 / m_height) * -2.0f + 1.0f;

                // Triangle 1
                vertexData.insert(vertexData.end(), { x1, y1, q.s0, q.t0 });
                vertexData.insert(vertexData.end(), { x2, y1, q.s1, q.t0 });
                vertexData.insert(vertexData.end(), { x1, y2, q.s0, q.t1 });
                // Triangle 2
                vertexData.insert(vertexData.end(), { x1, y2, q.s0, q.t1 });
                vertexData.insert(vertexData.end(), { x2, y1, q.s1, q.t0 });
                vertexData.insert(vertexData.end(), { x2, y2, q.s1, q.t1 });
            }
        }
        m_vertexCount = vertexData.size() / 4;

        if (m_vertexCount == 0) return;

        uint32_t requiredBufferSize = vertexData.size() * sizeof(float);
        if (requiredBufferSize > m_vertexBufferSize) {
            if (m_vertexBuffer) wgpuBufferRelease(m_vertexBuffer);
            m_vertexBufferSize = requiredBufferSize;
            m_vertexBuffer = init::create_buffer(m_device, m_vertexBufferSize, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        }

        wgpuQueueWriteBuffer(m_queue, m_vertexBuffer, 0, vertexData.data(), requiredBufferSize);

        // --- Record render commands ---
        wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, requiredBufferSize);
        wgpuRenderPassEncoderDraw(renderPass, m_vertexCount, 1, 0, 0);
    }

} // namespace flint::graphics
