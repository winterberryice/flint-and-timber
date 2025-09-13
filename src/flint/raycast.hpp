#pragma once

#include "player.hpp"
#include "world.hpp"
#include <optional>
#include <utility>

namespace flint
{
    enum class BlockFace
    {
        PosX,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ
    };

    std::optional<std::pair<glm::ivec3, BlockFace>> cast_ray(
        const Player &player,
        const World &world,
        float max_distance);
} // namespace flint
