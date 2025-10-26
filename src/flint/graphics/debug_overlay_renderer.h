#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "stb_truetype.h"

namespace flint {
namespace graphics {

class DebugOverlayRenderer {
public:
    DebugOverlayRenderer();

    void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, uint32_t width, uint32_t height);
    void cleanup();

    void toggle();
    void update(const glm::vec3& playerPosition);
    void render(WGPURenderPassEncoder renderPass);
    void onResize(uint32_t width, uint32_t height);

private:
    void createPipeline(WGPUTextureFormat surfaceFormat);
    void createFontTexture();
    void createBindGroup();

    // Member variables
    bool m_isVisible = false;
    glm::vec3 m_playerPosition;
    std::string m_textToRender;

    uint32_t m_width;
    uint32_t m_height;

    WGPUDevice m_device = nullptr;
    WGPUQueue m_queue = nullptr;
    WGPURenderPipeline m_pipeline = nullptr;

    // Font resources
    static constexpr int FONT_ATLAS_WIDTH = 512;
    static constexpr int FONT_ATLAS_HEIGHT = 512;
    // We bake the first 96 characters of the ASCII set (from character code 32).
    std::vector<stbtt_bakedchar> m_charData;

    WGPUTexture m_fontTexture = nullptr;
    WGPUTextureView m_fontTextureView = nullptr;
    WGPUSampler m_fontSampler = nullptr;
    WGPUBindGroup m_bindGroup = nullptr;

    // Vertex buffer for text rendering
    WGPUBuffer m_vertexBuffer = nullptr;
    uint32_t m_vertexBufferSize = 0;
    uint32_t m_vertexCount = 0;
};

} // namespace graphics
} // namespace flint
