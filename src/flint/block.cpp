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
            return true;
        case BlockType::Air:
        default:
            return false;
        }
    }

} // namespace flint