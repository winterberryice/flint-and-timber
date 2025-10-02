#pragma once

#include "world.h"
#include "player.h"
#include <glm/glm.hpp>
#include <optional>

namespace flint {

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

std::optional<RaycastResult> cast_ray(
    const Player& player,
    const World& world,
    float max_distance);

} // namespace flint