#pragma once

#include <webgpu/webgpu.h>
#include <cstdint>

namespace flint
{
    namespace graphics
    {
        class DebugMesh
        {
        public:
            DebugMesh();
            void generate(WGPUDevice device);
            void cleanup();
            void render(WGPURenderPassEncoder renderPass) const;

        private:
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            uint32_t m_indexCount = 0;
        };
    }
}