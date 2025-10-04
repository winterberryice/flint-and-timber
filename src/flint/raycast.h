#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace flint {

// Forward declarations
class Chunk;
namespace player {
    class Player;
}

enum class BlockFace {
    PosX, // +X
    NegX, // -X
    PosY, // +Y
    NegY, // -Y
    PosZ, // +Z
    NegZ, // -Z
};

struct RaycastResult {
    glm::ivec3 block_pos;
    BlockFace face;
};

// This version of cast_ray works with a single Chunk for simplicity.
std::optional<RaycastResult> cast_ray(
    const player::Player& player,
    const Chunk& chunk,
    float max_distance);

} // namespace flint