#include "world.hpp"
#include <cmath>
#include <algorithm>
#include <deque>

World::World() {}

Chunk* World::get_or_create_chunk(int chunk_x, int chunk_z) {
    auto it = chunks.find({chunk_x, chunk_z});
    if (it == chunks.end()) {
        it = chunks.emplace(std::piecewise_construct,
                            std::forward_as_tuple(chunk_x, chunk_z),
                            std::forward_as_tuple(chunk_x, chunk_z)).first;
        it->second.generate_terrain();
        it->second.calculate_sky_light();
    }
    return &it->second;
}

const Chunk* World::get_chunk(int chunk_x, int chunk_z) const {
    auto it = chunks.find({chunk_x, chunk_z});
    if (it != chunks.end()) {
        return &it->second;
    }
    return nullptr;
}

std::pair<std::pair<int, int>, std::tuple<int, int, int>> World::world_to_chunk_coords(float world_x, float world_y, float world_z) {
    int chunk_x = static_cast<int>(std::floor(world_x / CHUNK_WIDTH));
    int chunk_z = static_cast<int>(std::floor(world_z / CHUNK_DEPTH));
    int local_x = static_cast<int>(std::fmod(std::fmod(world_x, static_cast<float>(CHUNK_WIDTH)) + static_cast<float>(CHUNK_WIDTH), static_cast<float>(CHUNK_WIDTH)));
    int local_z = static_cast<int>(std::fmod(std::fmod(world_z, static_cast<float>(CHUNK_DEPTH)) + static_cast<float>(CHUNK_DEPTH), static_cast<float>(CHUNK_DEPTH)));
    int clamped_y = static_cast<int>(std::max(0.0f, std::min(world_y, static_cast<float>(CHUNK_HEIGHT - 1))));
    return {{chunk_x, chunk_z}, {local_x, clamped_y, local_z}};
}

std::optional<Block> World::get_block_at_world(float world_x, float world_y, float world_z) const {
    auto [chunk_coords, local_coords] = world_to_chunk_coords(world_x, world_y, world_z);
    auto [local_x, local_y, local_z] = local_coords;
    if (local_y >= CHUNK_HEIGHT) {
        return std::nullopt;
    }
    const Chunk* chunk = get_chunk(chunk_coords.first, chunk_coords.second);
    if (chunk) {
        return chunk->get_block(local_x, local_y, local_z);
    }
    return std::nullopt;
}

uint8_t World::get_light_level(glm::ivec3 pos) const {
    if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) {
        return 0;
    }
    auto block = get_block_at_world(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
    return block ? block->sky_light : 0;
}

void World::set_light_level(glm::ivec3 pos, uint8_t level) {
    if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) {
        return;
    }
    auto [chunk_coords, local_coords] = world_to_chunk_coords(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
    auto [local_x, local_y, local_z] = local_coords;

    auto it = chunks.find({chunk_coords.first, chunk_coords.second});
    if (it != chunks.end()) {
        it->second.set_block_light(local_x, local_y, local_z, level);
    }
}

bool World::is_block_transparent(glm::ivec3 pos) const {
    if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) {
        return true;
    }
    auto block = get_block_at_world(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
    return block ? block->is_transparent() : true;
}

void World::propagate_light_addition(glm::ivec3 new_air_block_pos) {
    uint8_t max_light_from_neighbors = 0;
    std::deque<glm::ivec3> light_propagation_queue;

    const std::array<glm::ivec3, 6> neighbors = {
        glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
        glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
    };

    for (const auto& offset : neighbors) {
        glm::ivec3 neighbor_pos = new_air_block_pos + offset;
        if (!is_block_transparent(neighbor_pos)) {
            continue;
        }
        uint8_t neighbor_light = get_light_level(neighbor_pos);
        if (neighbor_light == 0) {
            continue;
        }
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

    const std::array<glm::ivec3, 6> neighbors = {
        glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
        glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
    };

    while(!removal_queue.empty()){
        auto [pos, light_level] = removal_queue.front();
        removal_queue.pop_front();

        for(const auto& offset : neighbors){
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
    const std::array<glm::ivec3, 6> neighbors = {
        glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
        glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
    };
    for (const auto& offset : neighbors) {
        glm::ivec3 neighbor = pos + offset;
        uint8_t neighbor_light = get_light_level(neighbor);
        if (offset.y == 1 && neighbor_light == 15) {
            return true;
        }
        if (neighbor_light > get_light_level(pos)) {
            return true;
        }
    }
    return false;
}

void World::run_light_propagation_queue(std::deque<glm::ivec3>& queue) {
    const std::array<glm::ivec3, 6> neighbors = {
        glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
        glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
    };

    while (!queue.empty()) {
        glm::ivec3 pos = queue.front();
        queue.pop_front();

        uint8_t current_light = get_light_level(pos);
        uint8_t neighbor_light_level = (current_light > 0) ? current_light - 1 : 0;

        if (neighbor_light_level == 0 && current_light <=1) continue;

        for (const auto& offset : neighbors) {
            glm::ivec3 neighbor_pos = pos + offset;
            uint8_t potential_light = (offset.y == -1 && current_light == 15) ? 15 : neighbor_light_level;
            uint8_t neighbor_current_light = get_light_level(neighbor_pos);

            if (is_block_transparent(neighbor_pos) && potential_light > neighbor_current_light) {
                set_light_level(neighbor_pos, potential_light);
                queue.push_back(neighbor_pos);
            }
        }
    }
}

std::pair<int, int> World::set_block(glm::ivec3 world_block_pos, BlockType block_type) {
    if (world_block_pos.y < 0 || world_block_pos.y >= CHUNK_HEIGHT) {
        // Or throw an exception
        return {-1, -1};
    }

    bool old_block_was_transparent = is_block_transparent(world_block_pos);
    Block new_block(block_type);
    bool new_block_is_transparent = new_block.is_transparent();

    auto [chunk_coords, local_coords] = world_to_chunk_coords(static_cast<float>(world_block_pos.x), static_cast<float>(world_block_pos.y), static_cast<float>(world_block_pos.z));
    auto [local_x, local_y, local_z] = local_coords;

    if (old_block_was_transparent == new_block_is_transparent) {
        get_or_create_chunk(chunk_coords.first, chunk_coords.second)->set_block(local_x, local_y, local_z, block_type);
        return chunk_coords;
    }

    uint8_t light_level_removed = get_light_level(world_block_pos);

    Chunk* chunk = get_or_create_chunk(chunk_coords.first, chunk_coords.second);
    auto existing_block = chunk->get_block(local_x, local_y, local_z);
    if(existing_block && existing_block->block_type == BlockType::Bedrock){
        return chunk_coords; // Cannot replace bedrock
    }

    chunk->set_block(local_x, local_y, local_z, block_type);

    if (!old_block_was_transparent && new_block_is_transparent) {
        propagate_light_addition(world_block_pos);
    } else {
        propagate_light_removal(world_block_pos, light_level_removed);
    }

    return chunk_coords;
}
