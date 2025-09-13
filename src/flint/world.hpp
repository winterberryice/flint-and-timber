#pragma once

#include <map>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "chunk.hpp"

namespace flint
{
    // A comparator for glm::ivec2 so it can be used as a key in std::map
    struct ivec2_less
    {
        bool operator()(const glm::ivec2 &a, const glm::ivec2 &b) const
        {
            if (a.x < b.x)
                return true;
            if (a.x > b.x)
                return false;
            return a.y < b.y;
        }
    };

    // The World class manages the collection of Chunks.
    // TODO: Currently, it only generates a static grid of chunks at startup.
    // A future improvement would be to dynamically load/unload chunks around the player.
    class World
    {
    public:
        World();
        ~World();

        void generate_initial_chunks(WGPUDevice device);

        Chunk *get_chunk(int chunk_x, int chunk_z);
        const Chunk *get_chunk(int chunk_x, int chunk_z) const;

        Block get_block_at_world(int world_x, int world_y, int world_z) const;
        void set_block_at_world(WGPUDevice device, int world_x, int world_y, int world_z, Block block);

        void render(WGPURenderPassEncoder renderPass) const;

    private:
        Chunk *get_or_create_chunk(WGPUDevice device, int chunk_x, int chunk_z);

        std::map<glm::ivec2, Chunk, ivec2_less> m_chunks;
    };
} // namespace flint
