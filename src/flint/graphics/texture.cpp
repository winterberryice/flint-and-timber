#include "texture.hpp"
#include <iostream>
#include <vector>

// Define STB_IMAGE_IMPLEMENTATION in one C++ file before including it.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Assumes stb_image.h is in the include path

namespace flint
{
    namespace graphics
    {

        Texture::Texture() {}

        Texture::~Texture()
        {
            cleanup();
        }

        // Move constructor
        Texture::Texture(Texture &&other) noexcept
            : m_texture(other.m_texture), m_view(other.m_view), m_sampler(other.m_sampler)
        {
            other.m_texture = nullptr;
            other.m_view = nullptr;
            other.m_sampler = nullptr;
        }

        // Move assignment operator
        Texture &Texture::operator=(Texture &&other) noexcept
        {
            if (this != &other)
            {
                cleanup();
                m_texture = other.m_texture;
                m_view = other.m_view;
                m_sampler = other.m_sampler;
                other.m_texture = nullptr;
                other.m_view = nullptr;
                other.m_sampler = nullptr;
            }
            return *this;
        }


        void Texture::cleanup()
        {
            if (m_sampler)
            {
                wgpuSamplerRelease(m_sampler);
                m_sampler = nullptr;
            }
            if (m_view)
            {
                wgpuTextureViewRelease(m_view);
                m_view = nullptr;
            }
            if (m_texture)
            {
                wgpuTextureRelease(m_texture);
                m_texture = nullptr;
            }
        }

        Texture Texture::load_from_file(WGPUDevice device, WGPUQueue queue, const std::string &path)
        {
            int width, height, channels;
            // Force 4 channels (RGBA)
            unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 4);

            if (!data)
            {
                std::cerr << "Failed to load image: " << path << std::endl;
                return Texture(); // Return an empty texture
            }

            Texture tex;

            WGPUExtent3D textureSize = {
                .width = (uint32_t)width,
                .height = (uint32_t)height,
                .depthOrArrayLayers = 1};

            WGPUTextureDescriptor textureDesc = {};
            textureDesc.label = path.c_str();
            textureDesc.size = textureSize;
            textureDesc.mipLevelCount = 1;
            textureDesc.sampleCount = 1;
            textureDesc.dimension = WGPUTextureDimension_2D;
            textureDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
            textureDesc.viewFormatCount = 0;
            textureDesc.viewFormats = nullptr;

            tex.m_texture = wgpuDeviceCreateTexture(device, &textureDesc);

            WGPUImageCopyTexture imageCopyTexture = {};
            imageCopyTexture.texture = tex.m_texture;
            imageCopyTexture.mipLevel = 0;
            imageCopyTexture.origin = {0, 0, 0};
            imageCopyTexture.aspect = WGPUTextureAspect_All;

            WGPUTextureDataLayout textureDataLayout = {};
            textureDataLayout.offset = 0;
            textureDataLayout.bytesPerRow = 4 * (uint32_t)width;
            textureDataLayout.rowsPerImage = (uint32_t)height;

            wgpuQueueWriteTexture(queue, &imageCopyTexture, data, 4 * width * height, &textureDataLayout, &textureSize);

            stbi_image_free(data);

            // Create texture view
            WGPUTextureViewDescriptor viewDesc = {};
            viewDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            viewDesc.dimension = WGPUTextureViewDimension_2D;
            viewDesc.baseMipLevel = 0;
            viewDesc.mipLevelCount = 1;
            viewDesc.baseArrayLayer = 0;
            viewDesc.arrayLayerCount = 1;
            viewDesc.aspect = WGPUTextureAspect_All;
            tex.m_view = wgpuTextureCreateView(tex.m_texture, &viewDesc);

            // Create sampler
            WGPUSamplerDescriptor samplerDesc = {};
            samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
            samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
            samplerDesc.magFilter = WGPUFilterMode_Nearest;
            samplerDesc.minFilter = WGPUFilterMode_Nearest;
            samplerDesc.mipmapFilter = WGPUFilterMode_Nearest;
            samplerDesc.lodMinClamp = 0.0f;
            samplerDesc.lodMaxClamp = 1.0f;
            samplerDesc.compare = WGPUCompareFunction_Undefined;
            samplerDesc.maxAnisotropy = 1;
            tex.m_sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

            return tex;
        }

    } // namespace graphics
} // namespace flint
