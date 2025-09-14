#pragma once

#include <optional>
#include <utility>
#include "glm/glm.hpp"

namespace flint {

    // Forward declarations
    class Player;
    class World;

    enum class BlockFace {
        PosX, // +X face (East)
        NegX, // -X face (West)
        PosY, // +Y face (Top)
        NegY, // -Y face (Bottom)
        PosZ, // +Z face (South)
        NegZ, // -Z face (North)
    };

    std::optional<std::pair<glm::ivec3, BlockFace>> cast_ray(
        const Player& player,
        const World& world,
        float max_distance
    );

} // namespace flint
