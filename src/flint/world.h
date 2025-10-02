#pragma once

#include "chunk.h"
#include <glm/glm.hpp>
#include <optional>

namespace flint {

class World {
public:
    World();

    void generate();

    const Block* get_block_at_world(const glm::ivec3& position) const;

    // Provide access to the chunk for systems that need it (e.g., meshing)
    const Chunk& get_chunk() const;

private:
    Chunk chunk; // For now, the world is just one chunk.
};

} // namespace flint