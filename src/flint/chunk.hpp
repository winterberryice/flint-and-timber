#pragma once

#include "block.hpp"
#include <vector>
#include <optional>
#include <utility>

namespace flint
{
    const int CHUNK_WIDTH = 16;
    const int CHUNK_HEIGHT = 32;
    const int CHUNK_DEPTH = 16;

    class Chunk
    {
    public:
        std::pair<int, int> coord;

        Chunk(int x, int z);

        void generate_terrain();
        void calculate_sky_light();

        std::optional<Block> get_block(int x, int y, int z) const;
        void set_block(int x, int y, int z, BlockType type);
        void set_block_with_tree_id(int x, int y, int z, BlockType type, uint32_t tree_id);
        void set_block_light(int x, int y, int z, uint8_t level);

    private:
        std::vector<std::vector<std::vector<Block>>> blocks;
        void place_tree(int x, int y_base, int z, uint32_t tree_id);
    };
} // namespace flint
