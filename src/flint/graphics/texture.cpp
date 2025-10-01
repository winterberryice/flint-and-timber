#include "texture.h"
#include "atlas_bytes.hpp" // The generated header with our atlas's byte data.
#include "debug.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

namespace flint {
namespace graphics {

// Move constructor
Texture::Texture(Texture&& other) noexcept
    : texture(other.texture), texture_view(other.texture_view), sampler(other.sampler) {
    // Nullify the other's pointers to prevent double-free
    other.texture = nullptr;
    other.texture_view = nullptr;
    other.sampler = nullptr;
}

// Move assignment operator
Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        // Release existing resources
        if (texture_view) wgpuTextureViewRelease(texture_view);
        if (texture) wgpuTextureRelease(texture);
        if (sampler) wgpuSamplerRelease(sampler);

        // Move resources from other
        texture = other.texture;
        texture_view = other.texture_view;
        sampler = other.sampler;

        // Nullify the other's pointers
        other.texture = nullptr;
        other.texture_view = nullptr;
        other.sampler = nullptr;
    }
    return *this;
}

// Destructor
Texture::~Texture() {
    if (sampler) wgpuSamplerRelease(sampler);
    if (texture_view) wgpuTextureViewRelease(texture_view);
    if (texture) wgpuTextureRelease(texture);
}

Texture Texture::load_from_atlas_bytes(WGPUDevice device, WGPUQueue queue) {
    Texture tex;

    int width, height, channels;
    // stbi_load_from_memory decodes the image from the byte array in atlas_bytes.hpp
    unsigned char* pixels = stbi_load_from_memory(
        ATLAS_PNG_DATA,
        ATLAS_PNG_SIZE,
        &width,
        &height,
        &channels,
        4 // Force 4 channels (RGBA)
    );

    if (!pixels) {
        throw std::runtime_error("Failed to load texture from embedded atlas bytes.");
    }

    // --- Create Texture ---
    WGPUTextureDescriptor texture_desc{};
    texture_desc.nextInChain = nullptr;
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.size = { (uint32_t)width, (uint32_t)height, 1 };
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.format = WGPUTextureFormat_RGBA8Unorm; // RGBA, 8 bits per channel, normalized
    texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    texture_desc.viewFormatCount = 0;
    texture_desc.viewFormats = nullptr;

    tex.texture = wgpuDeviceCreateTexture(device, &texture_desc);
    if (!tex.texture) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create WGPU texture.");
    }

    // --- Write Pixel Data to Texture ---
    WGPUImageCopyTexture destination{};
    destination.texture = tex.texture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };
    destination.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout source_layout{};
    source_layout.offset = 0;
    source_layout.bytesPerRow = 4 * width;
    source_layout.rowsPerImage = height;

    WGPUExtent3D write_size = { (uint32_t)width, (uint32_t)height, 1 };

    wgpuQueueWriteTexture(queue, &destination, pixels, 4 * width * height, &source_layout, &write_size);

    stbi_image_free(pixels); // We're done with the CPU-side pixel data

    // --- Create Texture View ---
    WGPUTextureViewDescriptor view_desc{};
    view_desc.nextInChain = nullptr;
    view_desc.format = WGPUTextureFormat_RGBA8Unorm;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;

    tex.texture_view = wgpuTextureCreateView(tex.texture, &view_desc);
     if (!tex.texture_view) {
        throw std::runtime_error("Failed to create WGPU texture view.");
    }

    // --- Create Sampler ---
    WGPUSamplerDescriptor sampler_desc{};
    sampler_desc.addressModeU = WGPUAddressMode_Repeat;
    sampler_desc.addressModeV = WGPUAddressMode_Repeat;
    sampler_desc.addressModeW = WGPUAddressMode_Repeat;
    sampler_desc.magFilter = WGPUFilterMode_Nearest; // For pixelated look
    sampler_desc.minFilter = WGPUFilterMode_Nearest; // For pixelated look
    sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    sampler_desc.lodMinClamp = 0.0f;
    sampler_desc.lodMaxClamp = 1.0f;
    sampler_desc.compare = WGPUCompareFunction_Undefined;

    tex.sampler = wgpuDeviceCreateSampler(device, &sampler_desc);
     if (!tex.sampler) {
        throw std::runtime_error("Failed to create WGPU sampler.");
    }

    log_info("Successfully loaded texture atlas from embedded bytes.");

    return tex;
}

} // namespace graphics
} // namespace flint