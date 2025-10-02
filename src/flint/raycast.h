#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace flint {

// Forward declarations to break circular dependencies
class World;
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

// Function signature uses forward-declared types
std::optional<RaycastResult> cast_ray(
    const player::Player& player,
    const World& world,
    float max_distance);

} // namespace flint