#pragma once

#include <webgpu/webgpu.h>
#include <string>

namespace flint
{
    namespace graphics
    {
        class Texture
        {
        public:
            Texture();
            ~Texture();

            // Delete copy and move constructors/assignments
            Texture(const Texture &) = delete;
            Texture &operator=(const Texture &) = delete;
            Texture(Texture &&other) noexcept;
            Texture &operator=(Texture &&other) noexcept;

            static Texture load_from_file(WGPUDevice device, WGPUQueue queue, const std::string &path);

            WGPUTextureView get_view() const { return m_view; }
            WGPUSampler get_sampler() const { return m_sampler; }

        private:
            void cleanup();

            WGPUTexture m_texture = nullptr;
            WGPUTextureView m_view = nullptr;
            WGPUSampler m_sampler = nullptr;
        };
    } // namespace graphics
} // namespace flint
