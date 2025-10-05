#include "texture.h"

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

} // namespace flint::init