#pragma once

#include "../raycast.h"
#include "../vertex.h"
#include <webgpu/webgpu.h>
#include <optional>
#include <vector>

namespace flint
{
    class Chunk; // Forward declaration

    namespace graphics
    {
        class SelectionRenderer
        {
        public:
            SelectionRenderer();
            ~SelectionRenderer();

            void init(WGPUDevice device);
            void update_mesh(const RaycastResult &result, const Chunk &chunk);
            void clear_mesh();
            void render(WGPURenderPassEncoder pass) const;
            void cleanup();

        private:
            void create_buffers();

            WGPUDevice m_device = nullptr;
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            uint32_t m_indexCount = 0;

            std::optional<RaycastResult> m_lastResult;

            std::vector<flint::Vertex> m_vertices;
            std::vector<uint16_t> m_indices;
        };
    } // namespace graphics
} // namespace flint