#include "item_renderer.hpp"
#include "../block.hpp"
#include <vector>

namespace flint::ui
{
    namespace
    {
        const float ATLAS_WIDTH_IN_BLOCKS = 16.0f;
        const float ATLAS_HEIGHT_IN_BLOCKS = 1.0f;

        void add_quad(
            std::vector<UIVertex> &vertices,
            const std::array<glm::vec2, 4> &points,
            const std::array<glm::vec2, 4> &uvs,
            const glm::vec4 &color)
        {
            vertices.push_back({points[0], color, uvs[0]});
            vertices.push_back({points[3], color, uvs[3]});
            vertices.push_back({points[1], color, uvs[1]});
            vertices.push_back({points[1], color, uvs[1]});
            vertices.push_back({points[3], color, uvs[3]});
            vertices.push_back({points[2], color, uvs[2]});
        }

        void generate_item_vertices(
            ItemType item_type,
            BlockType block_type,
            glm::vec2 position,
            float size,
            glm::vec4 color,
            std::vector<UIVertex> &vertices)
        {
            Block temp_block(block_type);
            auto indices = temp_block.get_texture_atlas_indices();
            auto uv_top = indices[4];
            auto uv_side = indices[0];
            auto uv_front = indices[2];

            float texel_width = 1.0f / ATLAS_WIDTH_IN_BLOCKS;
            float texel_height = 1.0f / ATLAS_HEIGHT_IN_BLOCKS;

            float u_top = uv_top[0] * texel_width;
            float v_top = uv_top[1] * texel_height;
            float u_side = uv_side[0] * texel_width;
            float v_side = uv_side[1] * texel_height;
            float u_front = uv_front[0] * texel_width;
            float v_front = uv_front[1] * texel_height;

            std::array<glm::vec2, 4> top_face_uv = {{{u_top, v_top}, {u_top + texel_width, v_top}, {u_top + texel_width, v_top + texel_height}, {u_top, v_top + texel_height}}};
            std::array<glm::vec2, 4> side_face_uv = {{{u_side, v_side}, {u_side + texel_width, v_side}, {u_side + texel_width, v_side + texel_height}, {u_side, v_side + texel_height}}};
            std::array<glm::vec2, 4> front_face_uv = {{{u_front, v_front}, {u_front + texel_width, v_front}, {u_front + texel_width, v_front + texel_height}, {u_front, v_front + texel_height}}};

            float s = size / 2.0f;
            float y_squish = 0.5f;

            position.y += size / 4.0f;

            glm::vec2 p1 = {position.x, position.y + s * y_squish};
            glm::vec2 p2 = {position.x + s, position.y};
            glm::vec2 p3 = {position.x, position.y - s * y_squish};
            glm::vec2 p4 = {position.x - s, position.y};
            add_quad(vertices, {p1, p2, p3, p4}, top_face_uv, color);

            glm::vec2 p5 = p4;
            glm::vec2 p6 = p3;
            glm::vec2 p7 = {p3.x, p3.y - s};
            glm::vec2 p8 = {p4.x, p4.y - s};
            add_quad(vertices, {p5, p6, p7, p8}, side_face_uv, color);

            glm::vec2 p9 = p3;
            glm::vec2 p10 = p2;
            glm::vec2 p11 = {p2.x, p2.y - s};
            glm::vec2 p12 = {p3.x, p3.y - s};
            add_quad(vertices, {p9, p10, p11, p12}, front_face_uv, color);
        }
    }

    void ItemRenderer::initialize(
        wgpu::Device device,
        wgpu::TextureFormat texture_format)
    {
        // ... pipeline and buffer creation ...
    }

    void ItemRenderer::render(
        wgpu::Device device,
        wgpu::Queue queue,
        wgpu::RenderPassEncoder render_pass,
        const std::vector<std::tuple<ItemStack, glm::vec2, float, glm::vec4>> &items)
    {

        std::vector<UIVertex> vertices;
        for (const auto &[item_stack, position, size, color] : items)
        {
            generate_item_vertices(
                item_stack.item_type,
                item_stack.block_type,
                position,
                size,
                color,
                vertices);
        }

        if (vertices.empty())
        {
            return;
        }

        uint64_t required_size = vertices.size() * sizeof(UIVertex);
        if (required_size > buffer_size)
        {
            buffer_size = required_size; // In a real app, might want to have a growth strategy
            // ... re-create buffer ...
        }

        queue.writeBuffer(vertex_buffer, 0, vertices.data(), required_size);

        render_pass.setPipeline(render_pipeline);
        render_pass.setBindGroup(0, projection_bind_group, 0, nullptr);
        render_pass.setBindGroup(1, ui_texture_bind_group, 0, nullptr);
        render_pass.setVertexBuffer(0, vertex_buffer, 0, required_size);
        render_pass.draw(vertices.size(), 1, 0, 0);
    }
} // namespace flint::ui
