#pragma once

#include <optional>
#include <glm/glm.hpp>

namespace flint
{
    class Player; // Forward declaration
    class World;  // Forward declaration

    enum class BlockFace
    {
        PosX, // +X
        NegX, // -X
        PosY, // +Y
        NegY, // -Y
        PosZ, // +Z
        NegZ, // -Z
    };

    struct RaycastResult
    {
        glm::ivec3 block_position;
        BlockFace hit_face;
    };

    std::optional<RaycastResult> cast_ray(
        const Player &player,
        const World &world,
        float max_distance);

} // namespace flint
