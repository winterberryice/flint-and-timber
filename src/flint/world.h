#pragma once

#include "chunk.h"
#include <glm/glm.hpp>

namespace flint
{

    class World
    {
    public:
        World();

        Chunk *getChunk();
        const Chunk *getChunk() const;

        Block *getBlock(int x, int y, int z);
        const Block *getBlock(int x, int y, int z) const;

        bool setBlock(int x, int y, int z, BlockType type);

        bool is_solid(int x, int y, int z) const;

    private:
        Chunk m_chunk;
    };

} // namespace flint