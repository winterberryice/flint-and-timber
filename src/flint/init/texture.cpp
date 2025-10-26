#include "texture.h"
#include "utils.h"

namespace flint::init
{

    void create_depth_texture(
        WGPUDevice device,
        uint32_t width,
        uint32_t height,
        WGPUTextureFormat format,
        WGPUTexture *pTexture,
        WGPUTextureView *pTextureView)
    {
        WGPUTextureDescriptor depthTextureDesc = {};
        depthTextureDesc.dimension = WGPUTextureDimension_2D;
        depthTextureDesc.format = format;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = 1;
        depthTextureDesc.size = {width, height, 1};
        depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
        depthTextureDesc.viewFormatCount = 1;
        depthTextureDesc.viewFormats = &format;
        *pTexture = wgpuDeviceCreateTexture(device, &depthTextureDesc);

        WGPUTextureViewDescriptor depthTextureViewDesc = {};
        depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        depthTextureViewDesc.baseArrayLayer = 0;
        depthTextureViewDesc.arrayLayerCount = 1;
        depthTextureViewDesc.baseMipLevel = 0;
        depthTextureViewDesc.mipLevelCount = 1;
        depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
        depthTextureViewDesc.format = format;
        *pTextureView = wgpuTextureCreateView(*pTexture, &depthTextureViewDesc);
    }

    void create_texture(WGPUDevice device, WGPUQueue queue, const void *data, uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTexture *pTexture, WGPUTextureView *pTextureView)
    {
        WGPUTextureDescriptor textureDesc = {};
        textureDesc.nextInChain = nullptr;
        textureDesc.label = makeStringView("Font Texture");
        textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = {width, height, 1};
        textureDesc.format = format;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;

        *pTexture = wgpuDeviceCreateTexture(device, &textureDesc);

        WGPUTextureViewDescriptor textureViewDesc = {};
        textureViewDesc.nextInChain = nullptr;
        textureViewDesc.label = makeStringView("Font Texture View");
        textureViewDesc.format = format;
        textureViewDesc.dimension = WGPUTextureViewDimension_2D;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.aspect = WGPUTextureAspect_All;

        *pTextureView = wgpuTextureCreateView(*pTexture, &textureViewDesc);

        WGPUTexelCopyTextureInfo destination = {};
        destination.texture = *pTexture;
        destination.mipLevel = 0;
        destination.origin = {0, 0, 0};
        destination.aspect = WGPUTextureAspect_All;

        WGPUTexelCopyBufferLayout source = {};
        source.offset = 0;
        source.bytesPerRow = width;
        source.rowsPerImage = height;

        wgpuQueueWriteTexture(queue, &destination, data, width * height, &source, &textureDesc.size);
    }

    WGPUSampler create_sampler(WGPUDevice device)
    {
        WGPUSamplerDescriptor samplerDesc = {};
        samplerDesc.nextInChain = nullptr;
        samplerDesc.label = makeStringView("Font Sampler");
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.minFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        samplerDesc.lodMinClamp = 0.0f;
        samplerDesc.lodMaxClamp = 1.0f;
        samplerDesc.compare = WGPUCompareFunction_Undefined;
        samplerDesc.maxAnisotropy = 1;

        return wgpuDeviceCreateSampler(device, &samplerDesc);
    }

} // namespace flint::init
