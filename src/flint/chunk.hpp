#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <vector>

#include "graphics/mesh.hpp"

namespace flint
{
    const int CHUNK_WIDTH = 16;
    const int CHUNK_HEIGHT = 32;
    const int CHUNK_DEPTH = 16;

    enum class BlockType
    {
        Air,
        Dirt,
        Grass,
        Bedrock,
        OakLog,
        OakLeaves,
    };

    class Chunk
    {
    public:
        Chunk();
        ~Chunk();

        bool initialize(WGPUDevice device);
        void cleanup();

        void render(WGPURenderPassEncoder renderPass) const;

        bool is_solid(int x, int y, int z) const;

    private:
        void generateChunkData();
        void generateMesh();

        BlockType m_blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH];

        // CPU data
        std::vector<graphics::Vertex> m_vertices;
        std::vector<uint16_t> m_indices;

        // GPU buffers
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        WGPUDevice m_device = nullptr;
    };

} // namespace flint
