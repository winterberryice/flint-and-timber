#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <cstring> // For strlen

namespace {
    WGPUStringView makeStringView(const char* str) {
        return WGPUStringView {
            .data = str,
            .length = str ? strlen(str) : 0
        };
    }
}

namespace flint {
    namespace graphics {

        Texture::Texture() {}

        Texture::~Texture() {
            cleanup();
        }

        bool Texture::loadFromFile(WGPUDevice device, WGPUQueue queue, const std::string& path) {
            m_device = device;

            int width, height, channels;
            stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force 4 channels (RGBA)

            if (!pixels) {
                std::cerr << "Failed to load texture: " << path << std::endl;
                return false;
            }

            // Create texture
            WGPUTextureDescriptor textureDesc = {};
            textureDesc.nextInChain = nullptr;
            textureDesc.label = makeStringView(path.c_str());
            textureDesc.size = { (uint32_t)width, (uint32_t)height, 1 };
            textureDesc.mipLevelCount = 1;
            textureDesc.sampleCount = 1;
            textureDesc.dimension = WGPUTextureDimension_2D;
            textureDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
            textureDesc.viewFormatCount = 0;
            textureDesc.viewFormats = nullptr;

            m_texture = wgpuDeviceCreateTexture(device, &textureDesc);
            if (!m_texture) {
                std::cerr << "Failed to create texture" << std::endl;
                stbi_image_free(pixels);
                return false;
            }

            // Upload texture data using the simpler wgpuQueueWriteTexture
            WGPUTexelCopyTextureInfo destination = {};
            destination.texture = m_texture;
            destination.mipLevel = 0;
            destination.origin = { 0, 0, 0 };
            destination.aspect = WGPUTextureAspect_All;

            WGPUTexelCopyBufferLayout dataLayout = {};
            dataLayout.offset = 0;
            dataLayout.bytesPerRow = 4 * (uint32_t)width;
            dataLayout.rowsPerImage = (uint32_t)height;

            wgpuQueueWriteTexture(queue, &destination, pixels, (size_t)4 * (size_t)width * (size_t)height, &dataLayout, &textureDesc.size);

            stbi_image_free(pixels);

            // Create texture view
            WGPUTextureViewDescriptor viewDesc = {};
            viewDesc.nextInChain = nullptr;
            viewDesc.label = makeStringView("Texture View");
            viewDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            viewDesc.dimension = WGPUTextureViewDimension_2D;
            viewDesc.baseMipLevel = 0;
            viewDesc.mipLevelCount = 1;
            viewDesc.baseArrayLayer = 0;
            viewDesc.arrayLayerCount = 1;
            viewDesc.aspect = WGPUTextureAspect_All;

            m_textureView = wgpuTextureCreateView(m_texture, &viewDesc);
            if (!m_textureView) {
                std::cerr << "Failed to create texture view" << std::endl;
                cleanup();
                return false;
            }

            // Create sampler
            WGPUSamplerDescriptor samplerDesc = {};
            samplerDesc.nextInChain = nullptr;
            samplerDesc.label = makeStringView("Texture Sampler");
            samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
            samplerDesc.magFilter = WGPUFilterMode_Nearest;
            samplerDesc.minFilter = WGPUFilterMode_Nearest;
            samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
            samplerDesc.lodMinClamp = 0.0f;
            samplerDesc.lodMaxClamp = 1.0f;
            samplerDesc.compare = WGPUCompareFunction_Undefined;
            samplerDesc.maxAnisotropy = 1;

            m_sampler = wgpuDeviceCreateSampler(device, &samplerDesc);
            if (!m_sampler) {
                std::cerr << "Failed to create sampler" << std::endl;
                cleanup();
                return false;
            }

            return true;
        }

        void Texture::cleanup() {
            if (m_sampler) {
                wgpuSamplerRelease(m_sampler);
                m_sampler = nullptr;
            }
            if (m_textureView) {
                wgpuTextureViewRelease(m_textureView);
                m_textureView = nullptr;
            }
            if (m_texture) {
                wgpuTextureRelease(m_texture);
                m_texture = nullptr;
            }
        }

    } // namespace graphics
} // namespace flint
