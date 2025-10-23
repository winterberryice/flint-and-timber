#include "world.h"
#include "light.h"

namespace flint
{

    World::World()
    {
        m_chunk.generateTerrain();
        Light::calculate_sky_light(this);
    }

    Chunk *World::getChunk()
    {
        return &m_chunk;
    }

    const Chunk *World::getChunk() const
    {
        return &m_chunk;
    }

    Block *World::getBlock(int x, int y, int z)
    {
        return m_chunk.getBlock(x, y, z);
    }

    const Block *World::getBlock(int x, int y, int z) const
    {
        return m_chunk.getBlock(x, y, z);
    }

    bool World::setBlock(int x, int y, int z, BlockType type)
    {
        Block *old_block = getBlock(x, y, z);
        if (!old_block)
        {
            return false;
        }

        bool old_transparent = old_block->isTransparent();
        uint8_t old_light_level = old_block->sky_light;

        bool success = m_chunk.setBlock(x, y, z, type);
        if (!success)
        {
            return false;
        }

        Block *new_block = getBlock(x, y, z);
        bool new_transparent = new_block->isTransparent();

        if (old_transparent && !new_transparent)
        {
            Light::propagate_light_removal(this, x, y, z, old_light_level);
        }
        else if (!old_transparent && new_transparent)
        {
            Light::propagate_light_addition(this, x, y, z);
        }

        return true;
    }

    bool World::is_solid(int x, int y, int z) const
    {
        const Block *block = getBlock(x, y, z);
        return block && block->isSolid();
    }

} // namespace flint