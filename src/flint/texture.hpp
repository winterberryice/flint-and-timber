#pragma once

#include <webgpu/webgpu.hpp>
#include <string>
#include <vector>

namespace flint
{
    class Texture
    {
    public:
        wgpu::TextureView view;
        wgpu::Sampler sampler;

        static Texture load_from_memory(
            wgpu::Device device,
            wgpu::Queue queue,
            const unsigned char *bytes,
            int len,
            const std::string &label);

        static Texture create_placeholder(
            wgpu::Device device,
            wgpu::Queue queue,
            const std::string &label);
    };
} // namespace flint
