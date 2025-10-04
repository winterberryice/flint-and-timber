#pragma once

#include "webgpu/webgpu.h"
#include <cstdint>
#include <optional>
#include "glm/glm.hpp"

namespace flint
{
    namespace graphics
    {
        class SelectionRenderer
        {
        public:
            SelectionRenderer() = default;
            ~SelectionRenderer();

            void create_buffers(WGPUDevice device);

            void update(const std::optional<glm::ivec3> &selected_block);

            void draw(WGPURenderPassEncoder renderPass) const;

            void cleanup();

        private:
            // GPU buffers
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            WGPUDevice m_device = nullptr;

            uint32_t m_indexCount = 0;
            bool m_visible = false;

            std::optional<glm::ivec3> m_lastSelectedBlock;
        };
    } // namespace graphics
} // namespace flint