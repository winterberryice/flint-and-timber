#pragma once

#include <webgpu/webgpu.h>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

#include "../physics.h" // For AABB
#include "../vertex.h"   // For Vertex struct

namespace flint
{
    namespace graphics
    {
        class DebugMesh
        {
        public:
            DebugMesh();
            ~DebugMesh();

            void generate(WGPUDevice device, const physics::AABB& aabb, const glm::vec4& color);
            void cleanup();
            void render(WGPURenderPassEncoder renderPass) const;

        private:
            WGPUDevice m_device = nullptr;
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            uint32_t m_indexCount = 0;
        };
    }
}