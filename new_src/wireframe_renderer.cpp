#include "wireframe_renderer.hpp"
#include <vector>
#include "glm/gtc/matrix_transform.hpp"

namespace {
    const float MARGIN = 0.001f;
    const float QUAD_THICKNESS = 0.02f;

    void create_strip_quad(
        glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
        std::vector<WireframeVertex>& vertices,
        std::vector<uint16_t>& indices) {

        uint16_t base_vertex_index = static_cast<uint16_t>(vertices.size());
        vertices.push_back({p0});
        vertices.push_back({p1});
        vertices.push_back({p2});
        vertices.push_back({p3});
        indices.push_back(base_vertex_index);
        indices.push_back(base_vertex_index + 1);
        indices.push_back(base_vertex_index + 2);
        indices.push_back(base_vertex_index);
        indices.push_back(base_vertex_index + 2);
        indices.push_back(base_vertex_index + 3);
    }

    void generate_quads_for_face(
        glm::vec3 face_center_offset, glm::vec3 axis1, glm::vec3 axis2,
        std::vector<WireframeVertex>& vertices,
        std::vector<uint16_t>& indices) {

        const float m = MARGIN;
        const float t = QUAD_THICKNESS;
        const float h1 = 0.5f;
        const float h2 = 0.5f;

        // Quad 1
        glm::vec3 q1p0 = face_center_offset - axis2 * h2 + axis1 * (-h1 + m) + axis2 * m;
        glm::vec3 q1p1 = face_center_offset - axis2 * h2 + axis1 * (h1 - m) + axis2 * m;
        glm::vec3 q1p2 = face_center_offset - axis2 * h2 + axis1 * (h1 - m) + axis2 * (m + t);
        glm::vec3 q1p3 = face_center_offset - axis2 * h2 + axis1 * (-h1 + m) + axis2 * (m + t);
        create_strip_quad(q1p0, q1p1, q1p2, q1p3, vertices, indices);

        // Quad 2
        glm::vec3 q2p0 = face_center_offset + axis2 * h2 + axis1 * (-h1 + m) - axis2 * (m + t);
        glm::vec3 q2p1 = face_center_offset + axis2 * h2 + axis1 * (h1 - m) - axis2 * (m + t);
        glm::vec3 q2p2 = face_center_offset + axis2 * h2 + axis1 * (h1 - m) - axis2 * m;
        glm::vec3 q2p3 = face_center_offset + axis2 * h2 + axis1 * (-h1 + m) - axis2 * m;
        create_strip_quad(q2p0, q2p1, q2p2, q2p3, vertices, indices);

        // Quad 3
        glm::vec3 q3p0 = face_center_offset - axis1 * h1 + axis2 * (-h2 + m + t) + axis1 * m;
        glm::vec3 q3p1 = face_center_offset - axis1 * h1 + axis2 * (h2 - m - t) + axis1 * m;
        glm::vec3 q3p2 = face_center_offset - axis1 * h1 + axis2 * (h2 - m - t) + axis1 * (m + t);
        glm::vec3 q3p3 = face_center_offset - axis1 * h1 + axis2 * (-h2 + m + t) + axis1 * (m + t);
        create_strip_quad(q3p0, q3p1, q3p2, q3p3, vertices, indices);

        // Quad 4
        glm::vec3 q4p0 = face_center_offset + axis1 * h1 + axis2 * (-h2 + m + t) - axis1 * (m + t);
        glm::vec3 q4p1 = face_center_offset + axis1 * h1 + axis2 * (h2 - m - t) - axis1 * (m + t);
        glm::vec3 q4p2 = face_center_offset + axis1 * h1 + axis2 * (h2 - m - t) - axis1 * m;
        glm::vec3 q4p3 = face_center_offset + axis1 * h1 + axis2 * (-h2 + m + t) - axis1 * m;
        create_strip_quad(q4p0, q4p1, q4p2, q4p3, vertices, indices);
    }

    std::tuple<std::vector<WireframeVertex>, std::vector<uint16_t>, std::vector<std::tuple<BlockFace, uint32_t, uint32_t>>>
    generate_face_quads_cube_geometry() {
        std::vector<WireframeVertex> all_vertices;
        std::vector<uint16_t> all_indices;
        std::vector<std::tuple<BlockFace, uint32_t, uint32_t>> face_render_info;

        auto generate_and_record = [&](BlockFace face, glm::vec3 center_offset, glm::vec3 axis1, glm::vec3 axis2) {
            uint32_t start_index_count = all_indices.size();
            generate_quads_for_face(center_offset, axis1, axis2, all_vertices, all_indices);
            uint32_t num_new_indices = all_indices.size() - start_index_count;
            if (num_new_indices > 0) {
                face_render_info.emplace_back(face, start_index_count, num_new_indices);
            }
        };

        generate_and_record(BlockFace::PosX, {1.0f, 0.5f, 0.5f}, {0, 1, 0}, {0, 0, 1});
        generate_and_record(BlockFace::NegX, {0.0f, 0.5f, 0.5f}, {0, 1, 0}, {0, 0, 1});
        generate_and_record(BlockFace::PosY, {0.5f, 1.0f, 0.5f}, {1, 0, 0}, {0, 0, 1});
        generate_and_record(BlockFace::NegY, {0.5f, 0.0f, 0.5f}, {1, 0, 0}, {0, 0, 1});
        generate_and_record(BlockFace::PosZ, {0.5f, 0.5f, 1.0f}, {1, 0, 0}, {0, 1, 0});
        generate_and_record(BlockFace::NegZ, {0.5f, 0.5f, 0.0f}, {1, 0, 0}, {0, 1, 0});

        return {all_vertices, all_indices, face_render_info};
    }

    glm::ivec3 get_neighbor_offset(BlockFace face) {
        switch (face) {
            case BlockFace::PosX: return {1, 0, 0};
            case BlockFace::NegX: return {-1, 0, 0};
            case BlockFace::PosY: return {0, 1, 0};
            case BlockFace::NegY: return {0, -1, 0};
            case BlockFace::PosZ: return {0, 0, 1};
            case BlockFace::NegZ: return {0, 0, -1};
        }
        return {0,0,0};
    }
}

WireframeRenderer::WireframeRenderer(
    wgpu::Device device,
    wgpu::TextureFormat texture_format,
    wgpu::BindGroupLayout camera_bind_group_layout) {

    // ... (shader loading, pipeline layout, etc. will be filled in later)

    auto [vertices, indices, face_info] = generate_face_quads_cube_geometry();
    face_render_info = face_info;

    // ... (buffer creation will be filled in later)
}

void WireframeRenderer::update_selection(std::optional<glm::ivec3> position) {
    current_selected_block_pos = position;
    if (position) {
        model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(position.value()));
    }
}

void WireframeRenderer::draw(wgpu::RenderPassEncoder render_pass, wgpu::Queue queue, const World& world) {
    if (!current_selected_block_pos) return;

    queue.writeBuffer(model_uniform_buffer, 0, &model_matrix, sizeof(glm::mat4));

    render_pass.setPipeline(render_pipeline);
    render_pass.setBindGroup(1, model_bind_group, 0, nullptr);
    render_pass.setVertexBuffer(0, vertex_buffer);
    render_pass.setIndexBuffer(index_buffer, wgpu::IndexFormat::Uint16);

    for (const auto& [face, index_offset, num_indices] : face_render_info) {
        if (num_indices == 0) continue;

        glm::ivec3 neighbor_offset = get_neighbor_offset(face);
        glm::ivec3 neighbor_pos = current_selected_block_pos.value() + neighbor_offset;

        bool should_draw_face = true;
        auto neighbor_block = world.get_block_at_world(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z);
        if (neighbor_block && neighbor_block->is_solid()) {
            should_draw_face = false;
        }

        if (should_draw_face) {
            render_pass.drawIndexed(num_indices, 1, index_offset, 0, 0);
        }
    }
}
