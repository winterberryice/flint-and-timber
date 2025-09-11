#pragma once

#include <webgpu/webgpu.h>
#include <cstdint>

namespace flint
{
    namespace graphics
    {
        class Texture
        {
        public:
            Texture();
            ~Texture();

            // Load texture from an in-memory image file (e.g., PNG)
            bool initializeFromMemory(
                WGPUDevice device,
                WGPUQueue queue,
                const unsigned char* file_data,
                size_t file_data_len,
                const char* label = nullptr
            );

            void cleanup();

            WGPUTextureView getView() const { return m_textureView; }
            WGPUSampler getSampler() const { return m_sampler; }

        private:
            WGPUTexture m_texture = nullptr;
            WGPUTextureView m_textureView = nullptr;
            WGPUSampler m_sampler = nullptr;
        };
    } // namespace graphics
} // namespace flint
