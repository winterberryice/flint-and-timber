#include "light.h"
#include "chunk.h"

namespace flint
{

    void Light::calculate_sky_light(World *world)
    {
        Chunk *chunk = world->getChunk();
        // Phase 0: Reset
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (int z = 0; z < CHUNK_DEPTH; ++z)
                {
                    chunk->getBlock(x, y, z)->sky_light = 0;
                }
            }
        }

        std::queue<glm::ivec3> light_queue;

        // Phase 1: Vertical Sky Light Pass & Optimized Queue Seeding
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                for (int y = CHUNK_HEIGHT - 1; y >= 0; --y)
                {
                    Block *block = chunk->getBlock(x, y, z);
                    if (block && block->isTransparent())
                    {
                        block->sky_light = 15;
                        light_queue.push({x, y, z});
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        // Phase 2: Propagation Flood Fill
        while (!light_queue.empty())
        {
            glm::ivec3 pos = light_queue.front();
            light_queue.pop();

            uint8_t current_light_level = chunk->getBlock(pos.x, pos.y, pos.z)->sky_light;
            uint8_t neighbor_light_level = current_light_level > 0 ? current_light_level - 1 : 0;

            if (neighbor_light_level <= 0)
            {
                continue;
            }

            const glm::ivec3 neighbors[6] = {
                pos + glm::ivec3(-1, 0, 0),
                pos + glm::ivec3(1, 0, 0),
                pos + glm::ivec3(0, -1, 0),
                pos + glm::ivec3(0, 1, 0),
                pos + glm::ivec3(0, 0, -1),
                pos + glm::ivec3(0, 0, 1)};

            for (const auto &neighbor : neighbors)
            {
                Block *neighbor_block = world->getBlock(neighbor.x, neighbor.y, neighbor.z);
                if (neighbor_block && neighbor_block->isTransparent() && neighbor_block->sky_light < neighbor_light_level)
                {
                    neighbor_block->sky_light = neighbor_light_level;
                    light_queue.push(neighbor);
                }
            }
        }
    }

    void Light::propagate_light_addition(World *world, int x, int y, int z)
    {
        std::queue<glm::ivec3> light_queue;

        // Check the block above for sky light
        Block *block_above = world->getBlock(x, y + 1, z);
        uint8_t max_light = 0;
        if (block_above && block_above->sky_light == 15)
        {
            max_light = 15;
        }
        else
        {
            const glm::ivec3 neighbors[6] = {
                {x - 1, y, z},
                {x + 1, y, z},
                {x, y - 1, z},
                {x, y + 1, z},
                {x, y, z - 1},
                {x, y, z + 1}};

            for (const auto &neighbor : neighbors)
            {
                Block *neighbor_block = world->getBlock(neighbor.x, neighbor.y, neighbor.z);
                if (neighbor_block)
                {
                    max_light = std::max(max_light, (uint8_t)(neighbor_block->sky_light > 0 ? neighbor_block->sky_light - 1 : 0));
                }
            }
        }

        Block *block = world->getBlock(x, y, z);
        if (block && block->sky_light < max_light)
        {
            block->sky_light = max_light;
            light_queue.push({x, y, z});
        }
        else
        {
            return;
        }

        while (!light_queue.empty())
        {
            glm::ivec3 pos = light_queue.front();
            light_queue.pop();

            uint8_t current_light_level = world->getBlock(pos.x, pos.y, pos.z)->sky_light;

            const glm::ivec3 prop_neighbors[6] = {
                pos + glm::ivec3(-1, 0, 0),
                pos + glm::ivec3(1, 0, 0),
                pos + glm::ivec3(0, -1, 0),
                pos + glm::ivec3(0, 1, 0),
                pos + glm::ivec3(0, 0, -1),
                pos + glm::ivec3(0, 0, 1)};

            for (const auto &neighbor : prop_neighbors)
            {
                uint8_t propagated_light = (current_light_level == 15 && pos.y > neighbor.y) ? 15 : (current_light_level > 0 ? current_light_level - 1 : 0);

                if (propagated_light > 0)
                {
                    Block *neighbor_block = world->getBlock(neighbor.x, neighbor.y, neighbor.z);
                    if (neighbor_block && neighbor_block->isTransparent() && neighbor_block->sky_light < propagated_light)
                    {
                        neighbor_block->sky_light = propagated_light;
                        light_queue.push(neighbor);
                    }
                }
            }
        }
    }

    // Helper function to check if a block has an alternative light source
    static bool should_be_relit(World *world, const glm::ivec3 &pos)
    {
        uint8_t current_light = 0;
        Block *current_block_ptr = world->getBlock(pos.x, pos.y, pos.z);
        if (current_block_ptr)
        {
            current_light = current_block_ptr->sky_light;
        }

        const glm::ivec3 neighbor_offsets[6] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

        for (const auto &offset : neighbor_offsets)
        {
            glm::ivec3 neighbor_pos = pos + offset;
            Block *neighbor_block = world->getBlock(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z);
            if (neighbor_block)
            {
                uint8_t neighbor_light = neighbor_block->sky_light;
                // Check for sky light coming from directly above
                if (offset.y == 1 && neighbor_light == 15)
                {
                    return true;
                }
                // Check if any neighbor is brighter
                if (neighbor_light > current_light)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Light::propagate_light_removal(World *world, int x, int y, int z, uint8_t light_level)
    {
        if (light_level == 0)
        {
            return;
        }

        std::queue<std::pair<glm::ivec3, uint8_t>> removal_queue;
        removal_queue.push({{x, y, z}, light_level});

        Block *start_block = world->getBlock(x, y, z);
        if (start_block)
        {
            start_block->sky_light = 0;
        }

        std::queue<glm::ivec3> relight_queue;

        while (!removal_queue.empty())
        {
            auto [pos, light] = removal_queue.front();
            removal_queue.pop();

            const glm::ivec3 neighbors[6] = {
                pos + glm::ivec3(-1, 0, 0),
                pos + glm::ivec3(1, 0, 0),
                pos + glm::ivec3(0, -1, 0),
                pos + glm::ivec3(0, 1, 0),
                pos + glm::ivec3(0, 0, -1),
                pos + glm::ivec3(0, 0, 1)};

            for (const auto &neighbor : neighbors)
            {
                Block *neighbor_block = world->getBlock(neighbor.x, neighbor.y, neighbor.z);
                if (neighbor_block)
                {
                    uint8_t neighbor_light = neighbor_block->sky_light;
                    if (neighbor_light == 0)
                    {
                        continue;
                    }

                    if (neighbor_light < light)
                    {
                        neighbor_block->sky_light = 0;
                        removal_queue.push({neighbor, neighbor_light});
                    }
                    else
                    {
                        if (should_be_relit(world, neighbor))
                        {
                            relight_queue.push(neighbor);
                        }
                        else
                        {
                            neighbor_block->sky_light = 0;
                            removal_queue.push({neighbor, neighbor_light});
                        }
                    }
                }
            }
        }

        while (!relight_queue.empty())
        {
            glm::ivec3 pos = relight_queue.front();
            relight_queue.pop();

            propagate_light_addition(world, pos.x, pos.y, pos.z);
        }
    }

} // namespace flint