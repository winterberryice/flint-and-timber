#include "texture.hpp"
#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace flint
{
    namespace graphics
    {
        Texture::Texture()
            : m_texture(nullptr), m_textureView(nullptr), m_sampler(nullptr)
        {
        }

        Texture::~Texture()
        {
            cleanup();
        }

        void Texture::cleanup()
        {
            if (m_sampler)
            {
                wgpuSamplerRelease(m_sampler);
                m_sampler = nullptr;
            }
            if (m_textureView)
            {
                wgpuTextureViewRelease(m_textureView);
                m_textureView = nullptr;
            }
            if (m_texture)
            {
                wgpuTextureRelease(m_texture);
                m_texture = nullptr;
            }
        }

        bool Texture::initializeFromMemory(
            WGPUDevice device,
            WGPUQueue queue,
            const unsigned char* file_data,
            size_t file_data_len,
            const char* label)
        {
            int width, height, channels;
            unsigned char* pixel_data = stbi_load_from_memory(file_data, file_data_len, &width, &height, &channels, 4);

            if (!pixel_data)
            {
                std::cerr << "Failed to load image from memory: " << stbi_failure_reason() << std::endl;
                return false;
            }

            // Create texture
            WGPUTextureDescriptor textureDesc = {};
            textureDesc.nextInChain = nullptr;
            textureDesc.label = label;
            textureDesc.size = {(uint32_t)width, (uint32_t)height, 1};
            textureDesc.mipLevelCount = 1;
            textureDesc.sampleCount = 1;
            textureDesc.dimension = WGPUTextureDimension_2D;
            textureDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
            textureDesc.viewFormatCount = 0;
            textureDesc.viewFormats = nullptr;

            m_texture = wgpuDeviceCreateTexture(device, &textureDesc);
            if (!m_texture)
            {
                std::cerr << "Failed to create texture" << std::endl;
                stbi_image_free(pixel_data);
                return false;
            }

            // Upload data to texture
            WGPUImageCopyTexture destination = {};
            destination.texture = m_texture;
            destination.mipLevel = 0;
            destination.origin = {0, 0, 0};
            destination.aspect = WGPUTextureAspect_All;

            WGPUTextureDataLayout dataLayout = {};
            dataLayout.offset = 0;
            dataLayout.bytesPerRow = 4 * width;
            dataLayout.rowsPerImage = height;

            WGPUExtent3D writeSize = {(uint32_t)width, (uint32_t)height, 1};

            wgpuQueueWriteTexture(queue, &destination, pixel_data, 4 * width * height, &dataLayout, &writeSize);

            stbi_image_free(pixel_data);

            // Create texture view
            WGPUTextureViewDescriptor viewDesc = {};
            viewDesc.nextInChain = nullptr;
            viewDesc.label = label ? (std::string(label) + " View").c_str() : nullptr;
            viewDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            viewDesc.dimension = WGPUTextureViewDimension_2D;
            viewDesc.baseMipLevel = 0;
            viewDesc.mipLevelCount = 1;
            viewDesc.baseArrayLayer = 0;
            viewDesc.arrayLayerCount = 1;
            viewDesc.aspect = WGPUTextureAspect_All;

            m_textureView = wgpuTextureCreateView(m_texture, &viewDesc);
            if (!m_textureView)
            {
                std::cerr << "Failed to create texture view" << std::endl;
                cleanup();
                return false;
            }

            // Create sampler
            WGPUSamplerDescriptor samplerDesc = {};
            samplerDesc.nextInChain = nullptr;
            samplerDesc.label = label ? (std::string(label) + " Sampler").c_str() : nullptr;
            samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
            samplerDesc.magFilter = WGPUFilterMode_Nearest;
            samplerDesc.minFilter = WGPUFilterMode_Nearest;
            samplerDesc.mipmapFilter = WGPUFilterMode_Nearest;

            m_sampler = wgpuDeviceCreateSampler(device, &samplerDesc);
            if (!m_sampler)
            {
                std::cerr << "Failed to create sampler" << std::endl;
                cleanup();
                return false;
            }

            return true;
        }
    } // namespace graphics
} // namespace flint
