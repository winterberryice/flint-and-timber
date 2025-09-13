#pragma once

#include <array>
#include <optional>
#include <cstdint>

namespace flint
{

    enum class BlockType : uint8_t
    {
        Air,
        Dirt,
        Grass,
        Bedrock,
        OakLog,
        OakLeaves,
    };

    struct Block
    {
        BlockType type = BlockType::Air;
        std::optional<uint32_t> tree_id;
        uint8_t sky_light = 0;
        uint8_t block_light = 0;

        Block() = default;

        Block(BlockType block_type) : type(block_type) {}

        static Block new_with_tree_id(BlockType block_type, uint32_t tree_id)
        {
            Block b(block_type);
            b.tree_id = tree_id;
            return b;
        }

        bool is_solid() const
        {
            switch (type)
            {
            case BlockType::Air:
            case BlockType::OakLeaves:
                return false;
            default:
                return true;
            }
        }

        bool is_transparent() const
        {
            switch (type)
            {
            case BlockType::OakLeaves:
            case BlockType::Air:
                return true;
            default:
                return false;
            }
        }

        // Returns atlas indices (column, row) for each face: [Front, Back, Right, Left, Top, Bottom]
        std::array<std::array<float, 2>, 6> get_texture_atlas_indices() const
        {
            switch (type)
            {
            case BlockType::Dirt:
                return {{{2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 0.0f}}};
            case BlockType::Grass:
                return {{{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {2.0f, 0.0f}}};
            case BlockType::Bedrock:
                return {{{3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 0.0f}}};
            case BlockType::OakLog:
                return {{{4.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 0.0f}, {5.0f, 0.0f}, {5.0f, 0.0f}}};
            case BlockType::OakLeaves:
                return {{{6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f}}};
            case BlockType::Air:
            default:
                return {{{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}}};
            }
        }
    };

} // namespace flint
