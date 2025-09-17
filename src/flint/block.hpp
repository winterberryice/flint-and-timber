#pragma once

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
        BlockType type = BlockType::Air;
        uint8_t sky_light = 0;
        uint8_t block_light = 0;

        Block() = default;
        explicit Block(BlockType type);

        bool is_solid() const;
        bool is_transparent() const;
    };
}
