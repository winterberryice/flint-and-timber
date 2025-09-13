#pragma once

#include "../block.hpp"

enum class ItemType {
    Block,
};

struct ItemStack {
    ItemType item_type;
    BlockType block_type; // Only valid if item_type is Block
    uint8_t count;

    ItemStack(BlockType b_type, uint8_t count) :
        item_type(ItemType::Block), block_type(b_type), count(count) {}
};
