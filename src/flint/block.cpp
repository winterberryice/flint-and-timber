#include "block.h"

namespace flint
{

    Block::Block(BlockType block_type) : type(block_type) {}

    bool Block::isSolid() const
    {
        switch (type)
        {
        case BlockType::Grass:
        case BlockType::Dirt:
        case BlockType::OakLog:
            return true;
        case BlockType::Air:
        case BlockType::OakLeaves:
        default:
            return false;
        }
    }

} // namespace flint