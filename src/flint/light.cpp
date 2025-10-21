#include "light.h"

namespace flint
{
    void Light::calculate_sky_light(Chunk *chunk)
    {
        // Phase 0: Reset
        for (size_t x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (size_t y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (size_t z = 0; z < CHUNK_DEPTH; ++z)
                {
                    chunk->setBlockSkyLight(x, y, z, 0);
                }
            }
        }

        std::queue<std::tuple<size_t, size_t, size_t>> light_queue;

        // Phase 1: Vertical Sky Light Pass & Optimized Queue Seeding
        for (size_t x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (size_t z = 0; z < CHUNK_DEPTH; ++z)
            {
                for (int y = CHUNK_HEIGHT - 1; y >= 0; --y)
                {
                    Block *block = chunk->getBlock(x, y, z);
                    if (block && block->isTransparent())
                    {
                        block->sky_light = 15;
                        light_queue.push({x, (size_t)y, z});
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
            auto [x, y, z] = light_queue.front();
            light_queue.pop();

            uint8_t current_light_level = chunk->getBlock(x, y, z)->sky_light;
            uint8_t neighbor_light_level = current_light_level > 0 ? current_light_level - 1 : 0;

            if (neighbor_light_level <= 0)
            {
                continue;
            }

            const int neighbors[6][3] = {
                {(int)x - 1, (int)y, (int)z},
                {(int)x + 1, (int)y, (int)z},
                {(int)x, (int)y - 1, (int)z},
                {(int)x, (int)y + 1, (int)z},
                {(int)x, (int)y, (int)z - 1},
                {(int)x, (int)y, (int)z + 1}};

            for (const auto &neighbor : neighbors)
            {
                int nx = neighbor[0];
                int ny = neighbor[1];
                int nz = neighbor[2];

                if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < CHUNK_HEIGHT && nz >= 0 && nz < CHUNK_DEPTH)
                {
                    Block *neighbor_block = chunk->getBlock(nx, ny, nz);
                    if (neighbor_block->isTransparent() && neighbor_block->sky_light < neighbor_light_level)
                    {
                        neighbor_block->sky_light = neighbor_light_level;
                        light_queue.push({(size_t)nx, (size_t)ny, (size_t)nz});
                    }
                }
            }
        }
    }
} // namespace flint