#include "world.hpp"
#include "chunk.hpp" // Ensure chunk is fully defined
#include "constants.hpp"

namespace flint {
    World::World() = default;
    World::~World() = default;

    Chunk* World::get_or_create_chunk(int chunk_x, int chunk_z) {
        glm::ivec2 coord(chunk_x, chunk_z);
        if (chunks.find(coord) == chunks.end()) {
            auto new_chunk = std::make_unique<Chunk>(coord);
            new_chunk->generate_terrain();
        new_chunk->calculate_sky_light();
            chunks[coord] = std::move(new_chunk);
        }
        return chunks[coord].get();
    }

    Chunk* World::get_chunk(int chunk_x, int chunk_z) const {
        glm::ivec2 coord(chunk_x, chunk_z);
        auto it = chunks.find(coord);
        if (it != chunks.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void World::world_to_chunk_coords(const glm::vec3& world_pos, glm::ivec2& chunk_coord, glm::ivec3& local_coord) {
        chunk_coord.x = static_cast<int>(floor(world_pos.x / CHUNK_WIDTH));
        chunk_coord.y = static_cast<int>(floor(world_pos.z / CHUNK_DEPTH));

        int local_x = static_cast<int>(world_pos.x) % CHUNK_WIDTH;
        if (local_x < 0) local_x += CHUNK_WIDTH;

        int local_z = static_cast<int>(world_pos.z) % CHUNK_DEPTH;
        if (local_z < 0) local_z += CHUNK_DEPTH;

        local_coord.x = local_x;
        local_coord.y = static_cast<int>(world_pos.y);
        local_coord.z = local_z;
    }

    Block World::get_block_at_world(const glm::vec3& world_pos) const {
        if (world_pos.y < 0 || world_pos.y >= CHUNK_HEIGHT) {
            return Block(BlockType::Air);
        }

        glm::ivec2 chunk_coord;
        glm::ivec3 local_coord;
        world_to_chunk_coords(world_pos, chunk_coord, local_coord);

        const Chunk* chunk = get_chunk(chunk_coord.x, chunk_coord.y);
        if (chunk) {
            return chunk->get_block(local_coord.x, local_coord.y, local_coord.z);
        }
        return Block(BlockType::Air);
    }

    void World::set_block_at_world(const glm::vec3& world_pos, Block block) {
        if (world_pos.y < 0 || world_pos.y >= CHUNK_HEIGHT) {
            return;
        }

        glm::ivec2 chunk_coord;
        glm::ivec3 local_coord;
        world_to_chunk_coords(world_pos, chunk_coord, local_coord);

        Chunk* chunk = get_or_create_chunk(chunk_coord.x, chunk_coord.y);
        if (chunk) {
            chunk->set_block(local_coord.x, local_coord.y, local_coord.z, block);
            // TODO: Add logic to update lighting and notify neighboring chunks for mesh rebuild
        }
    }
}
