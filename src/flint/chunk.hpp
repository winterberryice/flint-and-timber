#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <vector>

#include "graphics/mesh.hpp"
#include "block.hpp"

namespace flint
{
    const int CHUNK_WIDTH = 16;
    const int CHUNK_HEIGHT = 32;
    const int CHUNK_DEPTH = 16;

    class Chunk
    {
    public:
        Chunk(int chunk_x = 0, int chunk_z = 0);
        ~Chunk();

        // Delete copy constructor and assignment operator
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;

        // Add move constructor and assignment operator
        Chunk(Chunk&& other) noexcept;
        Chunk& operator=(Chunk&& other) noexcept;

        bool initialize(WGPUDevice device);
        void cleanup();

        void render(WGPURenderPassEncoder renderPass) const;

        const Block &get_block(int x, int y, int z) const;
        void set_block(int x, int y, int z, Block block);
        void regenerate_mesh();

        glm::ivec2 get_world_position() const { return {m_chunk_x, m_chunk_z}; }

    private:
        void generateChunkData();
        void generateMesh();

        int m_chunk_x;
        int m_chunk_z;

        Block m_blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH];

        // CPU data
        std::vector<graphics::Vertex> m_vertices;
        std::vector<uint16_t> m_indices;

        // GPU buffers
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_indexBuffer = nullptr;
        WGPUDevice m_device = nullptr;
    };

} // namespace flint
