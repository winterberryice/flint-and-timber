#include "world.h"
#include <cmath>

namespace flint
{

    World::World() = default;

    Chunk *World::getOrCreateChunk(int chunkX, int chunkZ)
    {
        auto key = std::make_pair(chunkX, chunkZ);
        if (m_chunks.find(key) == m_chunks.end())
        {
            auto newChunk = std::make_unique<Chunk>(chunkX, chunkZ);
            newChunk->generateTerrain();
            m_chunks[key] = std::move(newChunk);
        }
        return m_chunks[key].get();
    }

    const Block *World::getBlock(glm::ivec3 position) const
    {
        auto [chunkCoords, localCoords] = worldToChunkCoordinates(position);
        auto key = std::make_pair(chunkCoords.x, chunkCoords.y);
        if (m_chunks.count(key))
        {
            return m_chunks.at(key)->getBlock(localCoords.x, localCoords.y, localCoords.z);
        }
        return nullptr;
    }

    bool World::setBlock(glm::ivec3 position, BlockType type)
    {
        if (position.y < 0 || position.y >= CHUNK_HEIGHT)
        {
            return false;
        }

        auto [chunkCoords, localCoords] = worldToChunkCoordinates(position);
        Chunk *chunk = getOrCreateChunk(chunkCoords.x, chunkCoords.y);
        return chunk->setBlock(localCoords.x, localCoords.y, localCoords.z, type);
    }

    std::pair<glm::ivec2, glm::ivec3> World::worldToChunkCoordinates(glm::vec3 position)
    {
        int chunkX = static_cast<int>(std::floor(position.x / CHUNK_WIDTH));
        int chunkZ = static_cast<int>(std::floor(position.z / CHUNK_DEPTH));

        int localX = static_cast<int>(position.x) % CHUNK_WIDTH;
        if (localX < 0)
            localX += CHUNK_WIDTH;
        int localZ = static_cast<int>(position.z) % CHUNK_DEPTH;
        if (localZ < 0)
            localZ += CHUNK_DEPTH;

        return {
            {chunkX, chunkZ},
            {localX, static_cast<int>(position.y), localZ}};
    }

} // namespace flint