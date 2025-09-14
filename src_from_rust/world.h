#pragma once

#include "chunk.h"
#include "block.h"
#include "glm/glm.hpp"
#include <unordered_map>
#include <utility>
#include <optional>
#include <deque>

namespace flint {

    // Custom hash for std::pair<int, int> to use it as a key in std::unordered_map
    struct PairHash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1,T2> &p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            // A common way to combine hashes
            return h1 ^ (h2 << 1);
        }
    };

    class World {
    public:
        World();

        Chunk* get_or_create_chunk(int chunk_x, int chunk_z);

        const Chunk* get_chunk(int chunk_x, int chunk_z) const;

        static std::pair<std::pair<int, int>, glm::ivec3> world_to_chunk_coords(float world_x, float world_y, float world_z);

        const Block* get_block_at_world(float world_x, float world_y, float world_z) const;

        std::optional<std::pair<int, int>> set_block(const glm::ivec3& world_block_pos, BlockType block_type);

    private:
        std::unordered_map<std::pair<int, int>, Chunk, PairHash> chunks;

        uint8_t get_light_level(glm::ivec3 pos) const;
        void set_light_level(glm::ivec3 pos, uint8_t level);
        bool is_block_transparent(glm::ivec3 pos) const;

        void propagate_light_addition(glm::ivec3 new_air_block_pos);
        void propagate_light_removal(glm::ivec3 new_solid_block_pos, uint8_t light_level_removed);
        bool should_be_relit(glm::ivec3 pos) const;
        void run_light_propagation_queue(std::deque<glm::ivec3>& queue);
    };

} // namespace flint
