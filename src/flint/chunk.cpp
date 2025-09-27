#include "chunk.h"

namespace flint
{

    // The constructor.
    // Because our `Block` struct has a default constructor that sets the type to Air,
    // the `m_blocks` array is automatically filled with Air blocks when a Chunk is created.
    // Therefore, the constructor body can be empty.
    Chunk::Chunk() {}

    void Chunk::generateTerrain()
    {
        const size_t surface_level = CHUNK_HEIGHT / 2;

        for (size_t x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (size_t z = 0; z < CHUNK_DEPTH; ++z)
            {
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
                    // Blocks above surface_level remain Air by default.
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

} // namespace flint