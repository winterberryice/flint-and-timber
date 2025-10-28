#pragma once

#include <webgpu/webgpu.h>
#include <string>

namespace flint::graphics
{
    class TextRenderer
    {
    public:
        void init(WGPUDevice device, WGPUTextureFormat surfaceFormat);
        void render(WGPURenderPassEncoder renderPass);
        void cleanup();

    private:
        void create_font_texture(WGPUDevice device);
        void create_vertex_buffer(WGPUDevice device);

        WGPURenderPipeline m_pipeline = nullptr;
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUTexture m_fontTexture = nullptr;
        WGPUTextureView m_fontTextureView = nullptr;
        WGPUSampler m_sampler = nullptr;
        WGPUBindGroup m_bindGroup = nullptr;

        int m_textureWidth = 0;
        int m_textureHeight = 0;
    };
}
