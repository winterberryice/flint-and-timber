#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>

#include "../selection_vertex.h"

namespace flint::graphics
{
    class SelectionMesh
    {
    public:
        SelectionMesh();
        ~SelectionMesh();

        void generate(WGPUDevice device, const glm::vec3& position);
        void render(WGPURenderPassEncoder renderPass) const;
        void cleanup();

    private:
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        uint32_t m_indexCount = 0;
    };
} // namespace flint::graphics