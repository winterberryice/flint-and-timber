#include "world.h"
#include "chunk.h"
#include "block.h"
#include <cmath>

namespace flint {

    World::World() {}

    Chunk* World::get_or_create_chunk(int chunk_x, int chunk_z) {
        auto it = chunks.find({chunk_x, chunk_z});
        if (it != chunks.end()) {
            return &it->second;
        }

        auto result = chunks.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(chunk_x, chunk_z),
                                     std::forward_as_tuple(chunk_x, chunk_z));
        Chunk* new_chunk = &result.first->second;
        new_chunk->generate_terrain();
        new_chunk->calculate_sky_light();
        return new_chunk;
    }

    const Chunk* World::get_chunk(int chunk_x, int chunk_z) const {
        auto it = chunks.find({chunk_x, chunk_z});
        if (it != chunks.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::pair<std::pair<int, int>, glm::ivec3> World::world_to_chunk_coords(float world_x, float world_y, float world_z) {
        int chunk_x = static_cast<int>(floor(world_x / CHUNK_WIDTH));
        int chunk_z = static_cast<int>(floor(world_z / CHUNK_DEPTH));

        // C++'s % can be weird with negatives, this ensures a positive result.
        float local_x_f = fmod(fmod(world_x, (float)CHUNK_WIDTH) + (float)CHUNK_WIDTH, (float)CHUNK_WIDTH);
        float local_z_f = fmod(fmod(world_z, (float)CHUNK_DEPTH) + (float)CHUNK_DEPTH, (float)CHUNK_DEPTH);

        int local_x = static_cast<int>(local_x_f);
        int local_z = static_cast<int>(local_z_f);
        int local_y = static_cast<int>(std::max(0.0f, std::min((float)CHUNK_HEIGHT - 1.0f, world_y)));

        return {{chunk_x, chunk_z}, {local_x, local_y, local_z}};
    }

    const Block* World::get_block_at_world(float world_x, float world_y, float world_z) const {
        auto [chunk_coords, local_coords] = world_to_chunk_coords(world_x, world_y, world_z);
        if (local_coords.y >= CHUNK_HEIGHT) return nullptr;
        const Chunk* chunk = get_chunk(chunk_coords.first, chunk_coords.second);
        if (chunk) {
            return chunk->get_block(local_coords.x, local_coords.y, local_coords.z);
        }
        return nullptr;
    }

    std::optional<std::pair<int, int>> World::set_block(const glm::ivec3& world_block_pos, BlockType block_type) {
        if (world_block_pos.y < 0 || world_block_pos.y >= CHUNK_HEIGHT) {
            return std::nullopt;
        }

        bool old_block_was_transparent = is_block_transparent(world_block_pos);
        // Create a temporary block to check its properties.
        Block new_block_info(block_type);
        bool new_block_is_transparent = new_block_info.is_transparent();

        auto [chunk_coords, local_coords] = world_to_chunk_coords(
            (float)world_block_pos.x, (float)world_block_pos.y, (float)world_block_pos.z
        );

        Chunk* chunk = get_or_create_chunk(chunk_coords.first, chunk_coords.second);

        const Block* existing_block = chunk->get_block(local_coords.x, local_coords.y, local_coords.z);
        if (existing_block && existing_block->block_type == BlockType::Bedrock) {
            return std::nullopt; // Cannot replace bedrock
        }

        uint8_t light_level_removed = get_light_level(world_block_pos);

        chunk->set_block(local_coords.x, local_coords.y, local_coords.z, block_type);

        if (old_block_was_transparent != new_block_is_transparent) {
            if (new_block_is_transparent) {
                propagate_light_addition(world_block_pos);
            } else {
                propagate_light_removal(world_block_pos, light_level_removed);
            }
        }

        return chunk_coords;
    }

    uint8_t World::get_light_level(glm::ivec3 pos) const {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) return 0;
        const Block* block = get_block_at_world((float)pos.x, (float)pos.y, (float)pos.z);
        return block ? block->sky_light : 0;
    }

    void World::set_light_level(glm::ivec3 pos, uint8_t level) {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) return;
        auto [chunk_coords, local_coords] = world_to_chunk_coords((float)pos.x, (float)pos.y, (float)pos.z);
        Chunk* chunk = get_or_create_chunk(chunk_coords.first, chunk_coords.second);
        chunk->set_block_light(local_coords.x, local_coords.y, local_coords.z, level);
    }

    bool World::is_block_transparent(glm::ivec3 pos) const {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) return true;
        const Block* block = get_block_at_world((float)pos.x, (float)pos.y, (float)pos.z);
        return block ? block->is_transparent() : true;
    }

    void World::propagate_light_addition(glm::ivec3 new_air_block_pos) {
        uint8_t max_light_from_neighbors = 0;
        std::deque<glm::ivec3> light_propagation_queue;

        const glm::ivec3 neighbors[] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };

        for (const auto& offset : neighbors) {
            glm::ivec3 neighbor_pos = new_air_block_pos + offset;
            if (!is_block_transparent(neighbor_pos)) continue;

            uint8_t neighbor_light = get_light_level(neighbor_pos);
            if (neighbor_light == 0) continue;

            uint8_t potential_light = (offset.y == 1 && neighbor_light == 15) ? 15 : neighbor_light - 1;
            if (potential_light > max_light_from_neighbors) {
                max_light_from_neighbors = potential_light;
            }
        }

        set_light_level(new_air_block_pos, max_light_from_neighbors);

        if (max_light_from_neighbors > 0) {
            light_propagation_queue.push_back(new_air_block_pos);
        }

        run_light_propagation_queue(light_propagation_queue);
    }

    void World::propagate_light_removal(glm::ivec3 new_solid_block_pos, uint8_t light_level_removed) {
        if (light_level_removed == 0) return;

        set_light_level(new_solid_block_pos, 0);

        std::deque<std::pair<glm::ivec3, uint8_t>> removal_queue;
        std::deque<glm::ivec3> relight_queue;
        removal_queue.push_back({new_solid_block_pos, light_level_removed});

        const glm::ivec3 neighbors[] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };

        while (!removal_queue.empty()) {
            auto [pos, light_level] = removal_queue.front();
            removal_queue.pop_front();

            for (const auto& offset : neighbors) {
                glm::ivec3 neighbor_pos = pos + offset;
                uint8_t neighbor_light = get_light_level(neighbor_pos);

                if (neighbor_light == 0) continue;

                if (neighbor_light < light_level) {
                    set_light_level(neighbor_pos, 0);
                    removal_queue.push_back({neighbor_pos, neighbor_light});
                } else {
                    if (should_be_relit(neighbor_pos)) {
                        relight_queue.push_back(neighbor_pos);
                    } else {
                        set_light_level(neighbor_pos, 0);
                        removal_queue.push_back({neighbor_pos, neighbor_light});
                    }
                }
            }
        }
        run_light_propagation_queue(relight_queue);
    }

    bool World::should_be_relit(glm::ivec3 pos) const {
        const glm::ivec3 neighbors[] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };
        for (const auto& offset : neighbors) {
            glm::ivec3 neighbor = pos + offset;
            uint8_t neighbor_light = get_light_level(neighbor);

            if (offset.y == 1 && neighbor_light == 15) return true;
            if (neighbor_light > get_light_level(pos)) return true;
        }
        return false;
    }

    void World::run_light_propagation_queue(std::deque<glm::ivec3>& queue) {
        const glm::ivec3 neighbors[] = {
            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
        };

        while (!queue.empty()) {
            glm::ivec3 pos = queue.front();
            queue.pop_front();

            uint8_t current_light = get_light_level(pos);
            uint8_t neighbor_light = (current_light > 0) ? current_light - 1 : 0;
            if (neighbor_light == 0) continue;

            for (const auto& offset : neighbors) {
                glm::ivec3 neighbor_pos = pos + offset;

                uint8_t potential_light = (offset.y == -1 && current_light == 15) ? 15 : neighbor_light;

                if (is_block_transparent(neighbor_pos) && potential_light > get_light_level(neighbor_pos)) {
                    set_light_level(neighbor_pos, potential_light);
                    queue.push_back(neighbor_pos);
                }
            }
        }
    }

} // namespace flint
