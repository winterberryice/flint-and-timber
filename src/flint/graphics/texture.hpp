#pragma once

#include <webgpu/webgpu.h>
#include <string>

namespace flint {
    namespace graphics {

        class Texture {
        public:
            Texture();
            ~Texture();

            // Load a texture from a memory buffer and upload it to the GPU
            bool loadFromMemory(WGPUDevice device, WGPUQueue queue, const unsigned char* buffer, unsigned int len);

            // Release GPU resources
            void cleanup();

            // Getters for the GPU resources
            WGPUTextureView getView() const { return m_textureView; }
            WGPUSampler getSampler() const { return m_sampler; }

        private:
            WGPUTexture m_texture = nullptr;
            WGPUTextureView m_textureView = nullptr;
            WGPUSampler m_sampler = nullptr;
            WGPUDevice m_device = nullptr; // To release resources
        };

    } // namespace graphics
} // namespace flint