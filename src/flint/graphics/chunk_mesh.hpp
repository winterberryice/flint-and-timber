#pragma once

#include "webgpu/webgpu.h"
#include "../chunk.h"
#include <vector>

namespace flint
{
    namespace graphics
    {
        class ChunkMesh
        {
        public:
            ChunkMesh();
            ~ChunkMesh();

            void generate(WGPUDevice device, const flint::Chunk &chunk);
            void render(WGPURenderPassEncoder renderPass) const;
            void cleanup();

        private:
            // GPU buffers
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            WGPUDevice m_device = nullptr;

            uint32_t m_indexCount = 0;
        };
    } // namespace graphics
} // namespace flint