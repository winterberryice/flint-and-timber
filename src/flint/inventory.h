#pragma once

#include "block.h"
#include <array>
#include <optional>

namespace flint
{
    namespace inventory
    {
        // Represents a stack of items in an inventory slot
        struct ItemStack
        {
            BlockType type;
            int count;

            ItemStack() : type(BlockType::Air), count(0) {}
            ItemStack(BlockType block_type, int stack_count = 1)
                : type(block_type), count(stack_count) {}

            bool isEmpty() const { return type == BlockType::Air || count <= 0; }
        };

        class Inventory
        {
        public:
            static constexpr int HOTBAR_SIZE = 9;
            static constexpr int INVENTORY_SIZE = 27;
            static constexpr int TOTAL_SIZE = HOTBAR_SIZE + INVENTORY_SIZE;

            Inventory();

            // Hotbar slot selection (0-8)
            void selectHotbarSlot(int slot);
            int getSelectedHotbarSlot() const { return selected_hotbar_slot; }

            // Get the block type in the selected hotbar slot
            std::optional<BlockType> getSelectedBlockType() const;

            // Slot access (0-35: 0-8 hotbar, 9-35 main inventory)
            ItemStack& getSlot(int slot);
            const ItemStack& getSlot(int slot) const;

            // Add item to inventory (returns false if inventory is full)
            bool addItem(BlockType type, int count = 1);

            // Remove item from inventory (returns false if not enough items)
            bool removeItem(BlockType type, int count = 1);

            // Check if inventory has an item
            bool hasItem(BlockType type, int count = 1) const;

            // Get total count of a specific block type
            int getItemCount(BlockType type) const;

        private:
            // Slots: 0-8 are hotbar, 9-35 are main inventory
            std::array<ItemStack, TOTAL_SIZE> slots;
            int selected_hotbar_slot; // 0-8
        };
    }
}
