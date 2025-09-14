#pragma once

#include <webgpu/webgpu.h>
#include <vector>
#include <string>
#include <cstdint>

namespace flint {

    class Texture {
    public:
        WGPUTextureView view = nullptr;
        WGPUSampler sampler = nullptr;

        // Note: The original Rust code used the `image` crate to decode byte slices.
        // This C++ version assumes the image data is already decoded into a raw RGBA byte vector.
        // A library like stb_image.h would typically be used here.
        static Texture load_from_rgba(
            WGPUDevice device,
            WGPUQueue queue,
            const std::vector<uint8_t>& rgba_data,
            uint32_t width,
            uint32_t height,
            const char* label
        );

        static Texture create_placeholder(
            WGPUDevice device,
            WGPUQueue queue,
            const char* label
        );

        // Destructor to release the resources
        ~Texture();

        // Delete copy constructor and assignment to prevent shallow copies
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Allow move construction and assignment
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

    private:
        // Private constructor to be used by static methods
        Texture() = default;
    };

} // namespace flint
