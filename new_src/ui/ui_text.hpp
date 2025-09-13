#pragma once

#include <webgpu/webgpu.hpp>
#include <string>
#include <vector>

// Placeholder for a text section
struct OwnedSection {
    // In a real implementation, this would contain text, font, color, position, etc.
    std::string text;
};

class UIText {
public:
    UIText(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height);

    void prepare(wgpu::Device device, wgpu::Queue queue, const std::vector<OwnedSection>& sections);
    void render(wgpu::RenderPassEncoder render_pass);
    void resize(uint32_t width, uint32_t height, wgpu::Queue queue);
};
