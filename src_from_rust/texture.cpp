#include "texture.h"
#include <iostream>

namespace flint {

    Texture Texture::load_from_rgba(
        WGPUDevice device,
        WGPUQueue queue,
        const std::vector<uint8_t>& rgba_data,
        uint32_t width,
        uint32_t height,
        const char* label
    ) {
        // TODO: Add error handling for empty data or zero dimensions.

        WGPUTextureDescriptor texture_desc = {};
        texture_desc.label = label;
        texture_desc.size = { width, height, 1 };
        texture_desc.mipLevelCount = 1;
        texture_desc.sampleCount = 1;
        texture_desc.dimension = WGPUTextureDimension_2D;
        texture_desc.format = WGPUTextureFormat_RGBA8UnormSrgb;
        texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

        WGPUTexture texture = wgpuDeviceCreateTexture(device, &texture_desc);

        WGPUImageCopyTexture destination = {};
        destination.texture = texture;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 };
        destination.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout source = {};
        source.offset = 0;
        source.bytesPerRow = 4 * width;
        source.rowsPerImage = height;

        wgpuQueueWriteTexture(queue, &destination, rgba_data.data(), rgba_data.size(), &source, &texture_desc.size);

        WGPUTextureViewDescriptor view_desc = {};
        view_desc.label = label;
        view_desc.format = texture_desc.format;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel = 0;
        view_desc.mipLevelCount = 1;
        view_desc.baseArrayLayer = 0;
        view_desc.arrayLayerCount = 1;

        WGPUSamplerDescriptor sampler_desc = {};
        sampler_desc.addressModeU = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeV = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeW = WGPUAddressMode_ClampToEdge;
        sampler_desc.magFilter = WGPUFilterMode_Nearest;
        sampler_desc.minFilter = WGPUFilterMode_Nearest;
        sampler_desc.mipmapFilter = WGPUFilterMode_Nearest;

        Texture result_texture;
        result_texture.view = wgpuTextureCreateView(texture, &view_desc);
        result_texture.sampler = wgpuDeviceCreateSampler(device, &sampler_desc);

        wgpuTextureRelease(texture); // The view and sampler hold references, so we can release the texture handle.

        return result_texture;
    }

    Texture Texture::create_placeholder(
        WGPUDevice device,
        WGPUQueue queue,
        const char* label
    ) {
        const uint32_t width = 16;
        const uint32_t height = 16;
        std::vector<uint8_t> rgba_data(4 * width * height);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                size_t idx = (y * width + x) * 4;
                if ((x / 4 + y / 4) % 2 == 0) {
                    rgba_data[idx] = 255; // R
                    rgba_data[idx + 1] = 0;   // G
                    rgba_data[idx + 2] = 255; // B
                    rgba_data[idx + 3] = 255; // A
                } else {
                    rgba_data[idx] = 0;   // R
                    rgba_data[idx + 1] = 0;   // G
                    rgba_data[idx + 2] = 0;   // B
                    rgba_data[idx + 3] = 255; // A
                }
            }
        }
        return load_from_rgba(device, queue, rgba_data, width, height, label);
    }

    Texture::~Texture() {
        if (view) wgpuTextureViewRelease(view);
        if (sampler) wgpuSamplerRelease(sampler);
    }

    Texture::Texture(Texture&& other) noexcept
        : view(other.view), sampler(other.sampler) {
        other.view = nullptr;
        other.sampler = nullptr;
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (view) wgpuTextureViewRelease(view);
            if (sampler) wgpuSamplerRelease(sampler);
            view = other.view;
            sampler = other.sampler;
            other.view = nullptr;
            other.sampler = nullptr;
        }
        return *this;
    }

} // namespace flint
