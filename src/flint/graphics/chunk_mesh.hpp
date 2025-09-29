#pragma once

#include "webgpu/webgpu.h"
#include "../chunk.h"
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include "../physics.h"

namespace flint
{
    namespace graphics
    {
        class ChunkMesh
        {
        public:
            ChunkMesh();
            ~ChunkMesh();

            void generate(WGPUDevice device, const flint::Chunk &chunk, const std::optional<physics::AABB> &debug_aabb = std::nullopt, const glm::vec4 &debug_color = {1.0f, 1.0f, 1.0f, 1.0f});
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