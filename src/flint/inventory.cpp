#include "inventory.h"
#include <algorithm>

namespace flint
{
    namespace inventory
    {
        Inventory::Inventory()
            : selected_hotbar_slot(0)
        {
            // Initialize all slots as empty
            for (auto& slot : slots)
            {
                slot = ItemStack();
            }

            // Give the player some starter items for testing
            slots[0] = ItemStack(BlockType::Grass, 64);
            slots[1] = ItemStack(BlockType::Dirt, 64);
            slots[2] = ItemStack(BlockType::OakLog, 32);
            slots[3] = ItemStack(BlockType::OakLeaves, 32);
        }

        void Inventory::selectHotbarSlot(int slot)
        {
            if (slot >= 0 && slot < HOTBAR_SIZE)
            {
                selected_hotbar_slot = slot;
            }
        }

        std::optional<BlockType> Inventory::getSelectedBlockType() const
        {
            const ItemStack& stack = slots[selected_hotbar_slot];
            if (stack.isEmpty())
            {
                return std::nullopt;
            }
            return stack.type;
        }

        ItemStack& Inventory::getSlot(int slot)
        {
            return slots[slot];
        }

        const ItemStack& Inventory::getSlot(int slot) const
        {
            return slots[slot];
        }

        bool Inventory::addItem(BlockType type, int count)
        {
            if (type == BlockType::Air || count <= 0)
            {
                return false;
            }

            int remaining = count;

            // First, try to stack with existing items
            for (auto& slot : slots)
            {
                if (slot.type == type)
                {
                    // TODO: Add max stack size limit (e.g., 64)
                    slot.count += remaining;
                    return true;
                }
            }

            // If no existing stack, find an empty slot
            for (auto& slot : slots)
            {
                if (slot.isEmpty())
                {
                    slot.type = type;
                    slot.count = remaining;
                    return true;
                }
            }

            return false; // Inventory is full
        }

        bool Inventory::removeItem(BlockType type, int count)
        {
            if (type == BlockType::Air || count <= 0)
            {
                return false;
            }

            if (!hasItem(type, count))
            {
                return false;
            }

            int remaining = count;

            for (auto& slot : slots)
            {
                if (slot.type == type && slot.count > 0)
                {
                    int to_remove = std::min(remaining, slot.count);
                    slot.count -= to_remove;
                    remaining -= to_remove;

                    if (slot.count <= 0)
                    {
                        slot.type = BlockType::Air;
                        slot.count = 0;
                    }

                    if (remaining <= 0)
                    {
                        return true;
                    }
                }
            }

            return remaining <= 0;
        }

        bool Inventory::hasItem(BlockType type, int count) const
        {
            return getItemCount(type) >= count;
        }

        int Inventory::getItemCount(BlockType type) const
        {
            int total = 0;
            for (const auto& slot : slots)
            {
                if (slot.type == type)
                {
                    total += slot.count;
                }
            }
            return total;
        }
    }
}
