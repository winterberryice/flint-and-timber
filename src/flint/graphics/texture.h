#pragma once

#include <webgpu/webgpu.h>
#include <string>

namespace flint {
namespace graphics {

class Texture {
public:
    // Factory function to create a texture from embedded atlas bytes.
    static Texture load_from_atlas_bytes(WGPUDevice device, WGPUQueue queue);

    // Destructor to release WebGPU resources.
    ~Texture();

    // Delete copy and move constructors/assignments to prevent resource mismanagement.
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Getters for the underlying WebGPU handles.
    WGPUTextureView get_view() const { return texture_view; }
    WGPUSampler get_sampler() const { return sampler; }

private:
    // Private constructor to be used by the factory function.
    Texture() = default;

    WGPUTexture texture = nullptr;
    WGPUTextureView texture_view = nullptr;
    WGPUSampler sampler = nullptr;
};

} // namespace graphics
} // namespace flint