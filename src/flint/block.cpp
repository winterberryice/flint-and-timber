#include "block.hpp"

namespace flint
{
    Block::Block(BlockType type)
        : block_type(type), tree_id(std::nullopt), sky_light(0), block_light(0) {}

    Block::Block(BlockType type, uint32_t tree_id_val)
        : block_type(type), tree_id(tree_id_val), sky_light(0), block_light(0) {}

    bool Block::is_solid() const
    {
        switch (block_type)
        {
        case BlockType::Air:
        case BlockType::OakLeaves:
            return false;
        default:
            return true;
        }
    }

    bool Block::is_transparent() const
    {
        switch (block_type)
        {
        case BlockType::OakLeaves:
        case BlockType::Air:
            return true;
        default:
            return false;
        }
    }

    std::array<std::array<float, 2>, 6> Block::get_texture_atlas_indices() const
    {
        switch (block_type)
        {
        case BlockType::Dirt:
            return {{{2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}}};
        case BlockType::Grass:
            return {{{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {2.0f, 0.0f}}};
        case BlockType::Bedrock:
            return {{{3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}}};
        case BlockType::Air:
            return {{{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}}};
        case BlockType::OakLog:
            return {{{4.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 0.0f}, {5.0f, 0.0f}, {5.0f, 0.0f}}};
        case BlockType::OakLeaves:
            return {{{6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}}};
        }
        // Should be unreachable
        return {{{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}}};
    }
} // namespace flint
