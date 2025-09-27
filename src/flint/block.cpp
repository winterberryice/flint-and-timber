#include "block.h"

namespace flint
{

    // Implementation of the constructor.
    Block::Block(BlockType block_type) : type(block_type) {}

    // Implementation of the isSolid method.
    bool Block::isSolid() const
    {
        // This simple check is the most direct translation of your `match` statement.
        return type != BlockType::Air;
    }

} // namespace flint