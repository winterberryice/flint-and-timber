#include "block.hpp"

namespace flint {
    Block::Block(BlockType type) : type(type) {}

    bool Block::is_solid() const {
        switch (type) {
            case BlockType::Air:
            case BlockType::OakLeaves:
                return false;
            default:
                return true;
        }
    }

    bool Block::is_transparent() const {
        switch (type) {
            case BlockType::Air:
            case BlockType::OakLeaves:
                return true;
            default:
                return false;
        }
    }
}
