#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <vector>

namespace flint
{
    namespace graphics
    {

        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color; // We'll use this for tinting, like for grass top
            glm::vec2 uv;    // Texture coordinates
        };

        class CubeMesh
        {
        public:
            CubeMesh();
            ~CubeMesh();

            // Initialize GPU buffers
            bool initialize(WGPUDevice device);
            void cleanup();

            // Render the cube
            void render(WGPURenderPassEncoder renderPass) const;

            // Get vertex data (useful for debugging)
            const std::vector<Vertex> &getVertices() const { return m_vertices; }
            const std::vector<uint16_t> &getIndices() const { return m_indices; }

        private:
            void createCubeData();

            // CPU data
            std::vector<Vertex> m_vertices;
            std::vector<uint16_t> m_indices;

            // GPU buffers
            WGPUBuffer m_vertexBuffer = nullptr;
            WGPUBuffer m_indexBuffer = nullptr;
            WGPUDevice m_device = nullptr;
        };

    } // namespace graphics
} // namespace flint
