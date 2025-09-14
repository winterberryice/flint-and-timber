#include "chunk.h"
#include <random>
#include <deque>

namespace flint {

    Chunk::Chunk(int coord_x, int coord_z)
        : coord(coord_x, coord_z) {
        blocks.resize(CHUNK_WIDTH, std::vector<std::vector<Block>>(
            CHUNK_HEIGHT, std::vector<Block>(
                CHUNK_DEPTH, Block(BlockType::Air)
            )
        ));
    }

    void Chunk::generate_terrain() {
        const int surface_level = CHUNK_HEIGHT / 2;

        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                blocks[x][0][z] = Block(BlockType::Bedrock);
                for (int y = 1; y < CHUNK_HEIGHT; ++y) {
                    if (y < surface_level) {
                        blocks[x][y][z] = Block(BlockType::Dirt);
                    } else if (y == surface_level) {
                        blocks[x][y][z] = Block(BlockType::Grass);
                    }
                }
            }
        }

        // TODO: The random number generation is not seeded, so it will be the same every time.
        // A global random engine or one passed in would be better.
        std::mt19937 rng( (coord.first << 16) | (coord.second & 0xFFFF) ); // Basic seed from chunk coord
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        uint32_t next_tree_id = 1;

        for (int x = 2; x < CHUNK_WIDTH - 2; ++x) {
            for (int z = 2; z < CHUNK_DEPTH - 2; ++z) {
                if (blocks[x][surface_level][z].block_type == BlockType::Grass) {
                    if (dist(rng) < 0.02) { // 2% chance for a tree
                        place_tree(x, surface_level + 1, z, next_tree_id);
                        next_tree_id++;
                        if (next_tree_id == 0) next_tree_id = 1; // Avoid 0
                    }
                }
            }
        }
    }

    void Chunk::place_tree(int x, int y_base, int z, uint32_t tree_id) {
        // TODO: This random engine is re-seeded every time. Not ideal.
        std::mt19937 rng( (coord.first << 16) | (coord.second & 0xFFFF) + y_base);
        std::uniform_int_distribution<int> trunk_dist(3, 5);
        std::uniform_real_distribution<double> place_dist(0.0, 1.0);

        int trunk_height = trunk_dist(rng);
        int canopy_radius = 2;
        int canopy_base_y_offset = std::max(0, trunk_height - 2);
        int tree_top_y = y_base + trunk_height;
        if (tree_top_y + 3 >= CHUNK_HEIGHT) return;

        for (int i = 0; i < trunk_height; ++i) {
            if (y_base + i < CHUNK_HEIGHT) {
                set_block(x, y_base + i, z, BlockType::OakLog);
            }
        }

        int canopy_center_y = y_base + trunk_height - 1;
        int y_start_canopy = std::max(0, y_base + canopy_base_y_offset);
        int y_end_canopy = std::min(CHUNK_HEIGHT - 1, y_base + trunk_height + 1);

        for (int ly = y_start_canopy; ly <= y_end_canopy; ++ly) {
            int y_dist = std::abs(ly - canopy_center_y);
            int r = (y_dist <= 1) ? canopy_radius : std::max(1, canopy_radius - 1);

            for (int lx = -r; lx <= r; ++lx) {
                for (int lz = -r; lz <= r; ++lz) {
                    int wx = x + lx;
                    int wz = z + lz;
                    if (wx < 0 || wx >= CHUNK_WIDTH || wz < 0 || wz >= CHUNK_DEPTH) continue;
                    if (lx == 0 && lz == 0 && ly < canopy_center_y) continue;
                    if (lx * lx + lz * lz > r * r) continue;

                    if (place_dist(rng) < 0.8) { // Simplified probability
                        if (blocks[wx][ly][wz].block_type == BlockType::Air) {
                            set_block_with_tree_id(wx, ly, wz, BlockType::OakLeaves, tree_id);
                        }
                    }
                }
            }
        }
    }

    const Block* Chunk::get_block(int x, int y, int z) const {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH) {
            return &blocks[x][y][z];
        }
        return nullptr;
    }

    Block* Chunk::get_block_mut(int x, int y, int z) {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH) {
            return &blocks[x][y][z];
        }
        return nullptr;
    }

    bool Chunk::set_block(int x, int y, int z, BlockType block_type) {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH) {
            blocks[x][y][z] = Block(block_type);
            return true;
        }
        return false;
    }

    bool Chunk::set_block_with_tree_id(int x, int y, int z, BlockType block_type, uint32_t tree_id) {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH) {
            blocks[x][y][z] = Block::new_with_tree_id(block_type, tree_id);
            return true;
        }
        return false;
    }

    void Chunk::calculate_sky_light() {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                for (int z = 0; z < CHUNK_DEPTH; ++z) {
                    blocks[x][y][z].sky_light = 0;
                }
            }
        }

        std::deque<glm::ivec3> light_queue;

        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
                    Block* block = &blocks[x][y][z];
                    if (block->is_transparent()) {
                        block->sky_light = 15;
                        light_queue.push_back({x, y, z});
                    } else {
                        break;
                    }
                }
            }
        }

        const glm::ivec3 neighbors[] = {
            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
        };

        while (!light_queue.empty()) {
            glm::ivec3 pos = light_queue.front();
            light_queue.pop_front();

            uint8_t current_light = get_block(pos.x, pos.y, pos.z)->sky_light;
            uint8_t neighbor_light = (current_light > 0) ? current_light - 1 : 0;
            if (neighbor_light == 0) continue;

            for (const auto& offset : neighbors) {
                glm::ivec3 neighbor_pos = pos + offset;
                if (neighbor_pos.x >= 0 && neighbor_pos.x < CHUNK_WIDTH &&
                    neighbor_pos.y >= 0 && neighbor_pos.y < CHUNK_HEIGHT &&
                    neighbor_pos.z >= 0 && neighbor_pos.z < CHUNK_DEPTH) {

                    Block* neighbor_block = get_block_mut(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z);
                    if (neighbor_block && neighbor_block->is_transparent() && neighbor_block->sky_light < neighbor_light) {
                        neighbor_block->sky_light = neighbor_light;
                        light_queue.push_back(neighbor_pos);
                    }
                }
                // TODO: Cross-chunk light propagation
            }
        }
    }

    void Chunk::set_block_light(int x, int y, int z, uint8_t sky_light) {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH) {
            blocks[x][y][z].sky_light = sky_light;
        }
    }

} // namespace flint
