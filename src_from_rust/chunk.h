#pragma once

#include "block.h"
#include "glm/glm.hpp"
#include <vector>
#include <utility>

namespace flint {

    constexpr int CHUNK_WIDTH = 16;
    constexpr int CHUNK_HEIGHT = 32;
    constexpr int CHUNK_DEPTH = 16;

    class Chunk {
    public:
        std::pair<int, int> coord;

        Chunk(int coord_x, int coord_z);

        void generate_terrain();

        const Block* get_block(int x, int y, int z) const;
        Block* get_block_mut(int x, int y, int z); // Added for mutable access

        bool set_block(int x, int y, int z, BlockType block_type);

        bool set_block_with_tree_id(int x, int y, int z, BlockType block_type, uint32_t tree_id);

        void calculate_sky_light();

        void set_block_light(int x, int y, int z, uint8_t sky_light);

    private:
        std::vector<std::vector<std::vector<Block>>> blocks;

        void place_tree(int x, int y_base, int z, uint32_t tree_id);
    };

} // namespace flint
