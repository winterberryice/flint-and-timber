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

        // Add hardcoded pillars for testing physics
        const size_t pillar_y_start = surface_level + 1;

        // Two 1-block high pillars
        setBlock(5, pillar_y_start, 5, BlockType::Dirt);
        setBlock(5, pillar_y_start, 7, BlockType::Dirt);

        // Two 2-block high pillars
        setBlock(7, pillar_y_start, 5, BlockType::Dirt);
        setBlock(7, pillar_y_start + 1, 5, BlockType::Dirt);
        setBlock(7, pillar_y_start, 7, BlockType::Dirt);
        setBlock(7, pillar_y_start + 1, 7, BlockType::Dirt);

        // Add a hardcoded tree
        const size_t tree_x = 4;
        const size_t tree_z = 4;
        const size_t trunk_height = 5;
        const size_t trunk_y_start = surface_level + 1;

        // Trunk
        for (size_t i = 0; i < trunk_height; ++i)
        {
            setBlock(tree_x, trunk_y_start + i, tree_z, BlockType::OakLog);
        }

        // Canopy
        const size_t canopy_y_start = trunk_y_start + trunk_height;
        for (int dx = -2; dx <= 2; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dz = -2; dz <= 2; ++dz)
                {
                    if (dx * dx + dy * dy + dz * dz <= 5)
                    {
                        // Avoid replacing the top of the trunk
                        if (dx == 0 && dz == 0 && dy <= 0) continue;

                        setBlock(tree_x + dx, canopy_y_start + dy, tree_z + dz, BlockType::OakLeaves);
                    }
                }
            }
        }
    }

    Block *Chunk::getBlock(int x, int y, int z)
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            return &m_blocks[x][y][z];
        }
        else
        {
            return nullptr;
        }
    }

    const Block *Chunk::getBlock(int x, int y, int z) const
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            return &m_blocks[x][y][z];
        }
        else
        {
            return nullptr;
        }
    }

    bool Chunk::setBlock(int x, int y, int z, BlockType type)
    {
        if (x >= 0 && x < CHUNK_WIDTH && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_DEPTH)
        {
            m_blocks[x][y][z] = Block(type);
            return true;
        }
        else
        {
            return false;
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