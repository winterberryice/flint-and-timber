#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>
#include <vector>

namespace flint
{
    Texture Texture::load_from_memory(
        wgpu::Device device,
        wgpu::Queue queue,
        const unsigned char *bytes,
        int len,
        const std::string &label)
    {

        int width, height, channels;
        unsigned char *data = stbi_load_from_memory(bytes, len, &width, &height, &channels, 4);
        if (!data)
        {
            throw std::runtime_error("Failed to load image: " + label);
        }

        wgpu::TextureDescriptor texture_desc;
        texture_desc.label = label.c_str();
        texture_desc.size = {(uint32_t)width, (uint32_t)height, 1};
        texture_desc.mipLevelCount = 1;
        texture_desc.sampleCount = 1;
        texture_desc.dimension = wgpu::TextureDimension::e2D;
        texture_desc.format = wgpu::TextureFormat::RGBA8UnormSrgb;
        texture_desc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

        wgpu::Texture texture = device.createTexture(&texture_desc);

        wgpu::ImageCopyTexture copy_texture_info;
        copy_texture_info.texture = texture;
        copy_texture_info.mipLevel = 0;
        copy_texture_info.origin = {0, 0, 0};
        copy_texture_info.aspect = wgpu::TextureAspect::All;

        wgpu::TextureDataLayout data_layout;
        data_layout.offset = 0;
        data_layout.bytesPerRow = 4 * width;
        data_layout.rowsPerImage = height;

        queue.writeTexture(copy_texture_info, data, width * height * 4, data_layout, texture_desc.size);

        stbi_image_free(data);

        wgpu::TextureViewDescriptor view_desc;
        view_desc.label = (label + " View").c_str();

        wgpu::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = wgpu::AddressMode::ClampToEdge;
        sampler_desc.addressModeV = wgpu::AddressMode::ClampToEdge;
        sampler_desc.addressModeW = wgpu::AddressMode::ClampToEdge;
        sampler_desc.magFilter = wgpu::FilterMode::Nearest;
        sampler_desc.minFilter = wgpu::FilterMode::Nearest;
        sampler_desc.mipmapFilter = wgpu::FilterMode::Nearest;

        Texture result_texture;
        result_texture.view = texture.createView(&view_desc);
        result_texture.sampler = device.createSampler(&sampler_desc);
        return result_texture;
    }

    Texture Texture::create_placeholder(
        wgpu::Device device,
        wgpu::Queue queue,
        const std::string &label)
    {

        const int width = 16, height = 16;
        std::vector<uint8_t> data(width * height * 4);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int idx = (y * width + x) * 4;
                if ((x / 4 + y / 4) % 2 == 0)
                {
                    data[idx] = 255;
                    data[idx + 1] = 0;
                    data[idx + 2] = 255;
                    data[idx + 3] = 255;
                }
                else
                {
                    data[idx] = 0;
                    data[idx + 1] = 0;
                    data[idx + 2] = 0;
                    data[idx + 3] = 255;
                }
            }
        }

        return load_from_memory(device, queue, data.data(), data.size(), label);
    }
} // namespace flint
