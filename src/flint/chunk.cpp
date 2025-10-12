#include "chunk.h"

namespace flint
{

    Chunk::Chunk(int chunkX, int chunkZ) : m_chunkX(chunkX), m_chunkZ(chunkZ)
    {
        // The m_blocks array is automatically filled with Air blocks.
    }

    void Chunk::generateTerrain()
    {
        for (size_t x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (size_t z = 0; z < CHUNK_DEPTH; ++z)
            {
                // Simple noise for terrain height
                int worldX = m_chunkX * CHUNK_WIDTH + x;
                int worldZ = m_chunkZ * CHUNK_DEPTH + z;
                double height = (std::sin(worldX * 0.1) + std::cos(worldZ * 0.1)) * 4 + (CHUNK_HEIGHT / 2);
                int surface_level = static_cast<int>(height);

                for (size_t y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    if (y < surface_level)
                    {
                        m_blocks[x][y][z] = Block(BlockType::Dirt);
                    }
                    else if (y == surface_level)
                    {
                        m_blocks[x][y][z] = Block(BlockType::Grass);
                    }
                }
            }
        }
    }

    const Block *Chunk::getBlock(size_t x, size_t y, size_t z) const
    {
        if (x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_DEPTH)
        {
            return &m_blocks[x][y][z];
        }
        else
        {
            return nullptr; // Equivalent to Rust's `None`
        }
    }

    bool Chunk::setBlock(size_t x, size_t y, size_t z, BlockType type)
    {
        if (x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_DEPTH)
        {
            m_blocks[x][y][z] = Block(type);
            return true; // Equivalent to Rust's `Ok(())`
        }
        else
        {
            return false; // Equivalent to Rust's `Err(...)`
        }
    }

    bool Chunk::is_solid(int x, int y, int z) const
    {
        // Check if the coordinates are within the chunk boundaries.
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            return false; // Treat out-of-bounds as not solid.
        }

        // Use the existing getBlock to check the block type.
        // We can safely cast to size_t because we've already checked for negative values.
        const Block *block = getBlock(static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z));

        // getBlock should not return nullptr for in-bounds coordinates, but check just in case.
        if (block)
        {
            return block->isSolid();
        }

        return false;
    }

} // namespace flint