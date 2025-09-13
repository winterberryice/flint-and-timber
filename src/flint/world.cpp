#include "world.hpp"
#include <stdexcept>

namespace flint
{
    World::World() {}

    World::~World() {}

    void World::generate_initial_chunks(WGPUDevice device)
    {
        // Generate a 3x3 grid of chunks around the origin
        for (int x = -1; x <= 1; ++x)
        {
            for (int z = -1; z <= 1; ++z)
            {
                get_or_create_chunk(device, x, z);
            }
        }
    }

    Chunk *World::get_chunk(int chunk_x, int chunk_z)
    {
        auto it = m_chunks.find({chunk_x, chunk_z});
        if (it != m_chunks.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const Chunk *World::get_chunk(int chunk_x, int chunk_z) const
    {
        auto it = m_chunks.find({chunk_x, chunk_z});
        if (it != m_chunks.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    Chunk *World::get_or_create_chunk(WGPUDevice device, int chunk_x, int chunk_z)
    {
        Chunk *chunk = get_chunk(chunk_x, chunk_z);
        if (chunk)
        {
            return chunk;
        }

        // Create a new chunk
        auto it = m_chunks.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(chunk_x, chunk_z),
                                   std::forward_as_tuple(chunk_x, chunk_z))
                            .first;
        chunk = &it->second;
        chunk->initialize(device); // This generates terrain and mesh
        return chunk;
    }

    Block World::get_block_at_world(int world_x, int world_y, int world_z) const
    {
        if (world_y < 0 || world_y >= CHUNK_HEIGHT)
        {
            return Block(BlockType::Air);
        }

        int chunk_x = static_cast<int>(std::floor(static_cast<float>(world_x) / CHUNK_WIDTH));
        int chunk_z = static_cast<int>(std::floor(static_cast<float>(world_z) / CHUNK_DEPTH));

        const Chunk *chunk = get_chunk(chunk_x, chunk_z);
        if (!chunk)
        {
            return Block(BlockType::Air); // Or some other default, like Bedrock below a certain y
        }

        int local_x = world_x - chunk_x * CHUNK_WIDTH;
        int local_z = world_z - chunk_z * CHUNK_DEPTH;

        return chunk->get_block(local_x, world_y, local_z);
    }

    void World::set_block_at_world(WGPUDevice device, int world_x, int world_y, int world_z, Block block)
    {
        if (world_y < 0 || world_y >= CHUNK_HEIGHT)
        {
            return;
        }

        int chunk_x = static_cast<int>(std::floor(static_cast<float>(world_x) / CHUNK_WIDTH));
        int chunk_z = static_cast<int>(std::floor(static_cast<float>(world_z) / CHUNK_DEPTH));

        Chunk *chunk = get_or_create_chunk(device, chunk_x, chunk_z);
        if (!chunk)
        {
            return;
        }

        int local_x = world_x - chunk_x * CHUNK_WIDTH;
        int local_z = world_z - chunk_z * CHUNK_DEPTH;

        chunk->set_block(local_x, world_y, local_z, block);
        chunk->regenerate_mesh();

        // Also regenerate neighbors if the block is on a border
        // TODO: This is a simplification. A more robust solution would check which face was affected.
        if (local_x == 0) {
            if (Chunk* neighbor = get_chunk(chunk_x - 1, chunk_z)) neighbor->regenerate_mesh();
        } else if (local_x == CHUNK_WIDTH - 1) {
            if (Chunk* neighbor = get_chunk(chunk_x + 1, chunk_z)) neighbor->regenerate_mesh();
        }
        if (local_z == 0) {
            if (Chunk* neighbor = get_chunk(chunk_x, chunk_z - 1)) neighbor->regenerate_mesh();
        } else if (local_z == CHUNK_DEPTH - 1) {
            if (Chunk* neighbor = get_chunk(chunk_x, chunk_z + 1)) neighbor->regenerate_mesh();
        }
    }

    void World::render(WGPURenderPassEncoder renderPass) const
    {
        for (const auto &pair : m_chunks)
        {
            pair.second.render(renderPass);
        }
    }

} // namespace flint
