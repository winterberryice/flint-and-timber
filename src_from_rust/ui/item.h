#pragma once

#include "../block.h"
#include <cstdint>

namespace flint {
namespace ui {

    // In Rust, ItemType is an enum with data.
    // enum ItemType { Block(BlockType) }
    // In C++, this can be modeled with std::variant.
    // For now, since there's only one type, we'll use a simple struct.
    // If more item types are added (e.g., tools), this should be
    // converted to a std::variant<BlockItem, ToolItem>.
    // For now, we assume all items are blocks.
    struct ItemType {
        BlockType block_type;

        // We need comparison operators for std::map or other containers.
        bool operator==(const ItemType& other) const {
            return block_type == other.block_type;
        }
        bool operator!=(const ItemType& other) const {
            return !(*this == other);
        }
        // For std::map
        bool operator<(const ItemType& other) const {
            return block_type < other.block_type;
        }
    };


    struct ItemStack {
        ItemType item_type;
        uint8_t count;

        ItemStack(ItemType type, uint8_t count);
    };

} // namespace ui
} // namespace flint
