#pragma once

#include "chunk.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>

namespace flint
{

    class World
    {
    public:
        World();

        Chunk *getOrCreateChunk(int chunkX, int chunkZ);

        const Block *getBlock(glm::ivec3 position) const;

        bool setBlock(glm::ivec3 position, BlockType type);

        static std::pair<glm::ivec2, glm::ivec3> worldToChunkCoordinates(glm::vec3 position);

    private:
        std::map<std::pair<int, int>, std::unique_ptr<Chunk>> m_chunks;
    };

} // namespace flint