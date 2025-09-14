#include "item.h"

namespace flint {
namespace ui {

    ItemStack::ItemStack(ItemType type, uint8_t count)
        : item_type(type), count(count) {}

} // namespace ui
} // namespace flint
