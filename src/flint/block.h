#pragma once

namespace flint
{

    // This is the direct C++ equivalent of your Rust `pub enum BlockType`.
    enum class BlockType
    {
        Air,
        Dirt,
        Grass
    };

    // This is the C++ equivalent of your Rust `pub struct Block`.
    struct Block
    {
        // Public member variable, equivalent to `pub block_type: BlockType`.
        BlockType type;

        // Constructor, equivalent to `Block::new`.
        explicit Block(BlockType block_type = BlockType::Air);

        // A const member function, equivalent to `is_solid(&self)`.
        bool isSolid() const;
    };

} // namespace flint