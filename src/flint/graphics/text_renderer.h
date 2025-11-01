#pragma once

#include <webgpu/webgpu.h>
#include <vector>
#include <string>
#include "render_pipeline.h"
#include "../vertex.h"

#include "stb_truetype.h"

namespace flint::graphics
{

    struct TextMeshData
    {
        std::vector<flint::Vertex> vertices;
        std::vector<uint32_t> indices;
    };


    class TextRenderer
    {
    public:
        TextRenderer();
        ~TextRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, int width, int height);
        void render(WGPURenderPassEncoder renderPass);
        void cleanup();
        void onResize(WGPUDevice device, WGPUQueue queue, int width, int height);

    private:
        void rebuild_mesh(WGPUDevice device, WGPUQueue queue);
        TextMeshData create_text_mesh(const std::string& text);

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        WGPUTexture m_fontTexture = nullptr;
        WGPUTextureView m_fontTextureView = nullptr;
        WGPUSampler m_fontSampler = nullptr;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        uint32_t m_indexCount = 0;

        int m_windowWidth = 0;
        int m_windowHeight = 0;

        stbtt_bakedchar* m_cdata = nullptr;
    };

} // namespace flint::graphics