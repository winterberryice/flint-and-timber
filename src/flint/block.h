#pragma once

#include <cstdint>

namespace flint
{

    // This is the direct C++ equivalent of your Rust `pub enum BlockType`.
    enum class BlockType
    {
        Air,
        Dirt,
    Grass,
    OakLog,
    OakLeaves
    };

    // This is the C++ equivalent of your Rust `pub struct Block`.
    struct Block
    {
        BlockType type;
        uint8_t sky_light = 0;

        // Constructor, equivalent to `Block::new`.
        explicit Block(BlockType block_type = BlockType::Air);

        // A const member function, equivalent to `is_solid(&self)`.
        bool isSolid() const;
        bool isTransparent() const;
    };

} // namespace flint