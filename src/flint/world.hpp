#pragma once

#include <unordered_map>
#include <memory>
#include "chunk.hpp"
#include "glm/glm.hpp"

namespace flint {

    // Hash function for glm::ivec2
    struct IVec2Hash {
        std::size_t operator()(const glm::ivec2& v) const {
            // A simple hash combination
            std::size_t h1 = std::hash<int>()(v.x);
            std::size_t h2 = std::hash<int>()(v.y);
            return h1 ^ (h2 << 1);
        }
    };

    class World {
    public:
        World();
        ~World();

        Chunk* get_or_create_chunk(int chunk_x, int chunk_z);
        Chunk* get_chunk(int chunk_x, int chunk_z) const;

        Block get_block_at_world(const glm::vec3& world_pos) const;
        void set_block_at_world(const glm::vec3& world_pos, Block block);

        static void world_to_chunk_coords(const glm::vec3& world_pos, glm::ivec2& chunk_coord, glm::ivec3& local_coord);

    private:
        std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> chunks;
    };
}
