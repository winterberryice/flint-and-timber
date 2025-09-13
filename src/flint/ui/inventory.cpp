#include "inventory.hpp"
#include <vector>

namespace flint::ui
{
    namespace
    {
        struct InventoryVertex
        {
            glm::vec2 position;
            glm::vec4 color;
        };
    }

    void Inventory::initialize(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height)
    {
        // ... implementation to be filled ...
    }

    void Inventory::render(wgpu::RenderPassEncoder render_pass, ItemRenderer &item_renderer)
    {
        // ... implementation to be filled ...
    }

    void Inventory::handle_mouse_click(const InputState &input, std::optional<ItemStack> &drag_item)
    {
        const float SLOT_SIZE = 50.0f;

        for (int i = 0; i < NUM_INVENTORY_SLOTS; ++i)
        {
            float slot_x = slot_positions[i].x - SLOT_SIZE / 2.0f;
            float slot_y = slot_positions[i].y - SLOT_SIZE / 2.0f;

            bool is_in_slot = input.cursor_position.x >= slot_x &&
                              input.cursor_position.x <= slot_x + SLOT_SIZE &&
                              input.cursor_position.y >= slot_y &&
                              input.cursor_position.y <= slot_y + SLOT_SIZE;

            if (is_in_slot)
            {
                if (input.left_mouse_pressed_this_frame)
                {
                    if (!drag_item.has_value())
                    {
                        if (items[i].has_value())
                        {
                            drag_item = items[i];
                            items[i].reset();
                        }
                    }
                }
                else if (input.left_mouse_released_this_frame)
                {
                    if (drag_item.has_value())
                    {
                        if (items[i].has_value())
                        {
                            if (items[i]->block_type == drag_item->block_type)
                            {
                                uint8_t total = items[i]->count + drag_item->count;
                                items[i]->count = std::min((uint8_t)64, total);
                                uint8_t remaining = total > 64 ? total - 64 : 0;
                                if (remaining > 0)
                                {
                                    drag_item->count = remaining;
                                }
                                else
                                {
                                    drag_item.reset();
                                }
                            }
                            else
                            {
                                std::swap(items[i], drag_item);
                            }
                        }
                        else
                        {
                            items[i] = drag_item;
                            drag_item.reset();
                        }
                    }
                }
                else if (input.right_mouse_released_this_frame)
                {
                    if (drag_item.has_value())
                    {
                        if (items[i].has_value())
                        {
                            if (items[i]->block_type == drag_item->block_type && items[i]->count < 64)
                            {
                                items[i]->count++;
                                drag_item->count--;
                                if (drag_item->count == 0)
                                {
                                    drag_item.reset();
                                }
                            }
                        }
                        else
                        {
                            items[i] = ItemStack{drag_item->block_type, 1};
                            drag_item->count--;
                            if (drag_item->count == 0)
                            {
                                drag_item.reset();
                            }
                        }
                    }
                    else if (items[i].has_value())
                    {
                        if (items[i]->count > 1)
                        {
                            uint8_t half = (items[i]->count + 1) / 2;
                            drag_item = ItemStack{items[i]->block_type, half};
                            items[i]->count -= half;
                        }
                    }
                }
                return;
            }
        }
    }
} // namespace flint::ui
