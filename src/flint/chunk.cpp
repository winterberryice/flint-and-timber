#include "chunk.hpp"
#include <vector>
#include <queue>

namespace flint {

    Chunk::Chunk(glm::ivec2 coord) : coord(coord) {
        blocks.resize(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, Block(BlockType::Air));
    }

    Chunk::~Chunk() = default;

    inline int Chunk::get_block_index(int x, int y, int z) {
        return y * (CHUNK_WIDTH * CHUNK_DEPTH) + z * CHUNK_WIDTH + x;
    }

    Block Chunk::get_block(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) {
            return Block(BlockType::Air);
        }
        return blocks[get_block_index(x, y, z)];
    }

    void Chunk::set_block(int x, int y, int z, Block block) {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) {
            return;
        }
        blocks[get_block_index(x, y, z)] = block;
    }

    void Chunk::generate_terrain() {
        int surface_level = CHUNK_HEIGHT / 2;

        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                set_block(x, 0, z, Block(BlockType::Bedrock));
                for (int y = 1; y < CHUNK_HEIGHT; ++y) {
                    if (y < surface_level) {
                        set_block(x, y, z, Block(BlockType::Dirt));
                    } else if (y == surface_level) {
                        set_block(x, y, z, Block(BlockType::Grass));
                    }
                }
            }
        }
        // Simplified terrain generation without trees for now.
    }

    void Chunk::calculate_sky_light() {
        // Phase 0: Reset all sky light levels to 0.
        for (auto& block : blocks) {
            block.sky_light = 0;
        }

        std::queue<glm::ivec3> light_queue;

        // Phase 1: Vertical Sky Light Pass & Queue Seeding
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
                    Block& block = blocks[get_block_index(x, y, z)];
                    if (block.is_transparent()) {
                        block.sky_light = 15;
                        light_queue.push({x, y, z});
                    } else {
                        break; // Stop at the first solid block from the top
                    }
                }
            }
        }

        // Phase 2: Propagation Flood Fill
        while (!light_queue.empty()) {
            glm::ivec3 pos = light_queue.front();
            light_queue.pop();

            uint8_t current_light_level = get_block(pos.x, pos.y, pos.z).sky_light;
            uint8_t neighbor_light_level = current_light_level - 1;

            if (neighbor_light_level <= 0) {
                continue;
            }

            const glm::ivec3 neighbors[] = {
                {pos.x - 1, pos.y, pos.z}, {pos.x + 1, pos.y, pos.z},
                {pos.x, pos.y - 1, pos.z}, {pos.x, pos.y + 1, pos.z},
                {pos.x, pos.y, pos.z - 1}, {pos.x, pos.y, pos.z + 1}
            };

            for (const auto& neighbor_pos : neighbors) {
                if (neighbor_pos.x >= 0 && neighbor_pos.x < CHUNK_WIDTH &&
                    neighbor_pos.y >= 0 && neighbor_pos.y < CHUNK_HEIGHT &&
                    neighbor_pos.z >= 0 && neighbor_pos.z < CHUNK_DEPTH) {

                    Block& neighbor_block = blocks[get_block_index(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z)];
                    if (neighbor_block.is_transparent() && neighbor_block.sky_light < neighbor_light_level) {
                        neighbor_block.sky_light = neighbor_light_level;
                        light_queue.push(neighbor_pos);
                    }
                }
                // Note: Cross-chunk propagation would be handled by the World class
            }
        }
    }
#include "world.hpp"

namespace {
    // Cube face vertices relative to the block's center (0.5, 0.5, 0.5)
    // Each face has 4 vertices
    const std::vector<flint::graphics::Vertex> cube_vertices = {
        // Front face (+Z)
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0},
        // Back face (-Z)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.5f, 0.0f}, 0},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 0.5f, 0.0f}, 0},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.5f, 0.0f}, 0},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.5f, 0.0f}, 0},
        // Right face (+X)
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 0},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 0},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0},
        // Left face (-X)
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.5f}, 0},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.5f}, 0},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.5f}, 0},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.5f}, 0},
        // Top face (+Y)
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, 0},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, 0},
        // Bottom face (-Y)
        {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 1.0f}, 0},
        {{0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 1.0f}, 0},
        {{0.5f, -0.5f, 0.5f}, {0.5f, 0.0f, 1.0f}, 0},
        {{-0.5f, -0.5f, 0.5f}, {0.5f, 0.0f, 1.0f}, 0},
    };

    const std::vector<uint16_t> cube_indices = {
        0, 1, 2, 0, 2, 3, // Front
        4, 5, 6, 4, 6, 7, // Back
        8, 9, 10, 8, 10, 11, // Right
        12, 13, 14, 12, 14, 15, // Left
        16, 17, 18, 16, 18, 19, // Top
        20, 21, 22, 20, 22, 23, // Bottom
    };

    glm::vec3 get_block_color(BlockType type) {
        switch (type) {
            case BlockType::Grass: return {0.0f, 0.8f, 0.1f};
            case BlockType::Dirt: return {0.5f, 0.25f, 0.05f};
            case BlockType::Bedrock: return {0.5f, 0.5f, 0.5f};
            case BlockType::OakLog: return {0.4f, 0.2f, 0.0f};
            case BlockType::OakLeaves: return {0.1f, 0.5f, 0.1f};
            default: return {1.0f, 0.0f, 1.0f}; // Magenta for errors
        }
    }
}

graphics::ChunkMeshData Chunk::build_mesh(const World* world) const {
    graphics::ChunkMeshData mesh_data;
    uint16_t vertex_offset = 0;

    glm::vec3 chunk_world_origin(coord.x * CHUNK_WIDTH, 0, coord.y * CHUNK_DEPTH);

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                Block current_block = get_block(x, y, z);
                if (current_block.type == BlockType::Air) {
                    continue;
                }

                glm::vec3 current_block_world_pos = chunk_world_origin + glm::vec3(x, y, z);

                // Check 6 neighbors
                const glm::ivec3 neighbor_offsets[] = {
                    {0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}
                };

                for (int i = 0; i < 6; ++i) {
                    glm::vec3 neighbor_pos = current_block_world_pos + glm::vec3(neighbor_offsets[i]);
                    Block neighbor_block = world->get_block_at_world(neighbor_pos);

                    if (neighbor_block.is_transparent()) {
                        // Face is visible, add its vertices and indices
                        for (int j = 0; j < 4; ++j) {
                            graphics::Vertex vertex = cube_vertices[i * 4 + j];
                            vertex.position += current_block_world_pos + glm::vec3(0.5f, 0.5f, 0.5f);
                            vertex.color = get_block_color(current_block.type);
                            vertex.sky_light = neighbor_block.sky_light;
                            mesh_data.vertices.push_back(vertex);
                        }
                        for (int j = 0; j < 6; ++j) {
                            mesh_data.indices.push_back(cube_indices[i * 6 + j] + vertex_offset);
                        }
                        vertex_offset += 4;
                    }
                }
            }
        }
    }

    return mesh_data;
}

}
