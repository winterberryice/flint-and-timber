#include "world.h"

namespace flint {

World::World() {
    // Constructor
}

void World::generate() {
    chunk.generateTerrain();
}

const Block* World::get_block_at_world(const glm::ivec3& position) const {
    // For now, we assume the world is only one chunk at (0,0,0).
    // We can directly use the world coordinates as local chunk coordinates.
    if (position.x >= 0 && position.x < CHUNK_WIDTH &&
        position.y >= 0 && position.y < CHUNK_HEIGHT &&
        position.z >= 0 && position.z < CHUNK_DEPTH) {
        return chunk.getBlock(position.x, position.y, position.z);
    }
    return nullptr;
}

} // namespace flint