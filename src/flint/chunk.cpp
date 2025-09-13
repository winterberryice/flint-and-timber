#include "chunk.hpp"
#include <random>
#include <deque>
#include <cmath>
#include <tuple>
#include <algorithm>

namespace flint
{
    Chunk::Chunk(int x, int z) : coord(x, z)
    {
        blocks.resize(CHUNK_WIDTH, std::vector<std::vector<Block>>(CHUNK_HEIGHT, std::vector<Block>(CHUNK_DEPTH, Block(BlockType::Air))));
    }

    void Chunk::generate_terrain()
    {
        int surface_level = CHUNK_HEIGHT / 2;

        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                blocks[x][0][z] = Block(BlockType::Bedrock);
                for (int y = 1; y < CHUNK_HEIGHT; ++y)
                {
                    if (y < surface_level)
                    {
                        blocks[x][y][z] = Block(BlockType::Dirt);
                    }
                    else if (y == surface_level)
                    {
                        blocks[x][y][z] = Block(BlockType::Grass);
                    }
                }
            }
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        uint32_t next_tree_id = 1;

        for (int x = 2; x < CHUNK_WIDTH - 2; ++x)
        {
            for (int z = 2; z < CHUNK_DEPTH - 2; ++z)
            {
                if (blocks[x][surface_level][z].block_type == BlockType::Grass)
                {
                    if (dis(gen) < 0.02)
                    {
                        place_tree(x, surface_level + 1, z, next_tree_id);
                        next_tree_id = (next_tree_id + 1);
                        if (next_tree_id == 0)
                            next_tree_id = 1;
                    }
                }
            }
        }
    }

    void Chunk::place_tree(int x, int y_base, int z, uint32_t tree_id)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> trunk_dist(3, 5);
        int trunk_height = trunk_dist(gen);
        int canopy_radius = 2;
        int canopy_base_y_offset = std::max(0, trunk_height - 2);
        int canopy_max_height_above_base = 3;

        int tree_top_y = y_base + trunk_height;
        if (tree_top_y + canopy_max_height_above_base >= CHUNK_HEIGHT)
        {
            return;
        }

        for (int i = 0; i < trunk_height; ++i)
        {
            if (y_base + i < CHUNK_HEIGHT)
            {
                set_block_with_tree_id(x, y_base + i, z, BlockType::OakLog, tree_id);
            }
        }

        int canopy_center_y_world = y_base + trunk_height - 1;
        int y_start_canopy = std::max(0, y_base + canopy_base_y_offset);
        int y_end_canopy = std::min(CHUNK_HEIGHT - 1, y_base + trunk_height + 1);

        for (int ly_world = y_start_canopy; ly_world <= y_end_canopy; ++ly_world)
        {
            int y_dist_from_canopy_center = std::abs(ly_world - canopy_center_y_world);
            int current_layer_radius = (y_dist_from_canopy_center <= 1) ? canopy_radius : std::max(1, canopy_radius - 1);

            for (int lx_offset = -current_layer_radius; lx_offset <= current_layer_radius; ++lx_offset)
            {
                for (int lz_offset = -current_layer_radius; lz_offset <= current_layer_radius; ++lz_offset)
                {
                    int current_x_world = x + lx_offset;
                    int current_z_world = z + lz_offset;

                    if (current_x_world < 0 || current_x_world >= CHUNK_WIDTH ||
                        current_z_world < 0 || current_z_world >= CHUNK_DEPTH)
                    {
                        continue;
                    }

                    if (lx_offset == 0 && lz_offset == 0 && ly_world < canopy_center_y_world)
                    {
                        continue;
                    }

                    int dist_sq_horiz = lx_offset * lx_offset + lz_offset * lz_offset;
                    if (dist_sq_horiz > current_layer_radius * current_layer_radius)
                    {
                        continue;
                    }

                    std::uniform_real_distribution<> dis(0.0, 1.0);
                    if (dis(gen) < 0.6)
                    { // Simplified probability
                        if (blocks[current_x_world][ly_world][current_z_world].block_type == BlockType::Air)
                        {
                            set_block_with_tree_id(current_x_world, ly_world, current_z_world, BlockType::OakLeaves, tree_id);
                        }
                    }
                }
            }
        }
    }

    std::optional<Block> Chunk::get_block(int x, int y, int z) const
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            return blocks[x][y][z];
        }
        return std::nullopt;
    }

    void Chunk::set_block(int x, int y, int z, BlockType type)
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            blocks[x][y][z] = Block(type);
        }
    }

    void Chunk::set_block_with_tree_id(int x, int y, int z, BlockType type, uint32_t tree_id)
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            blocks[x][y][z] = Block(type, tree_id);
        }
    }

    void Chunk::calculate_sky_light()
    {
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (int z = 0; z < CHUNK_DEPTH; ++z)
                {
                    blocks[x][y][z].sky_light = 0;
                }
            }
        }

        std::deque<std::tuple<int, int, int>> light_queue;

        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                for (int y = CHUNK_HEIGHT - 1; y >= 0; --y)
                {
                    if (blocks[x][y][z].is_transparent())
                    {
                        blocks[x][y][z].sky_light = 15;
                        light_queue.push_back({x, y, z});
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        while (!light_queue.empty())
        {
            auto [x, y, z] = light_queue.front();
            light_queue.pop_front();

            uint8_t current_light_level = blocks[x][y][z].sky_light;
            uint8_t neighbor_light_level = (current_light_level > 0) ? current_light_level - 1 : 0;

            if (neighbor_light_level == 0)
                continue;

            const std::array<std::tuple<int, int, int>, 6> neighbors = {{
                {x - 1, y, z},
                {x + 1, y, z},
                {x, y - 1, z},
                {x, y + 1, z},
                {x, y, z - 1},
                {x, y, z + 1},
            }};

            for (const auto &[nx, ny, nz] : neighbors)
            {
                if (nx >= 0 && nx < CHUNK_WIDTH && ny >= 0 && ny < CHUNK_HEIGHT && nz >= 0 && nz < CHUNK_DEPTH)
                {
                    if (blocks[nx][ny][nz].is_transparent() && blocks[nx][ny][nz].sky_light < neighbor_light_level)
                    {
                        blocks[nx][ny][nz].sky_light = neighbor_light_level;
                        light_queue.push_back({nx, ny, nz});
                    }
                }
            }
        }
    }

    void Chunk::set_block_light(int x, int y, int z, uint8_t level)
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            blocks[x][y][z].sky_light = level;
        }
    }
} // namespace flint
