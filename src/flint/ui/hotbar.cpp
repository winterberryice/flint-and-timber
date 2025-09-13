#include "hotbar.hpp"
#include <vector>
#include "glm/gtc/matrix_transform.hpp"

namespace flint::ui
{
    namespace
    {
        struct HotbarVertex
        {
            glm::vec2 position;
            glm::vec4 color;
        };

        const char *shader_source = R"(
            struct VertexInput {
                @location(0) position: vec2<f32>,
                @location(1) color: vec4<f32>,
            };

            struct VertexOutput {
                @builtin(position) clip_position: vec4<f32>,
                @location(0) color: vec4<f32>,
            };

            struct ProjectionUniform {
                projection_matrix: mat4x4<f32>,
            };

            @group(0) @binding(0)
            var<uniform> u_projection: ProjectionUniform;

            @vertex
            fn vs_main(model: VertexInput) -> VertexOutput {
                var out: VertexOutput;
                out.clip_position = u_projection.projection_matrix * vec4<f32>(model.position, 0.0, 1.0);
                out.color = model.color;
                return out;
            }

            @fragment
            fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
                return in.color;
            }
        )";
    }

    void Hotbar::initialize(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height)
    {
        // ... implementation to be filled ...
    }

    void Hotbar::render(wgpu::RenderPassEncoder render_pass, ItemRenderer &item_renderer)
    {
        // ... implementation to be filled ...
    }

    void Hotbar::handle_mouse_click(const InputState &input, std::optional<ItemStack> &drag_item)
    {
        const float SLOT_SIZE = 50.0f;

        for (int i = 0; i < NUM_HOTBAR_SLOTS; ++i)
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
