#pragma once

#include <optional>
#include <array>
#include <cstdint>

namespace flint {
    enum class BlockType {
        Air,
        Dirt,
        Grass,
        Bedrock,
        OakLog,
        OakLeaves,
    };

    struct Block {
        BlockType block_type;
        std::optional<uint32_t> tree_id;
        uint8_t sky_light;
        uint8_t block_light;

        Block(BlockType type);

        static Block new_with_tree_id(BlockType block_type, uint32_t tree_id);

        bool is_solid() const;

        bool is_transparent() const;

        std::array<std::array<float, 2>, 6> get_texture_atlas_indices() const;
    };
}
