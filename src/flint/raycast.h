#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

namespace flint
{
    class Chunk; // Forward declaration
    namespace player
    {
        class Player; // Forward declaration
    }
    class Camera; // Forward declaration

    // Represents the face of a block that was hit by a raycast.
    enum class BlockFace
    {
        Front,
        Back,
        Left,
        Right,
        Top,
        Bottom
    };

    // Utility function to convert a BlockFace enum to a string for debugging.
    inline std::string to_string(BlockFace face)
    {
        switch (face)
        {
        case BlockFace::Front:
            return "Front";
        case BlockFace::Back:
            return "Back";
        case BlockFace::Left:
            return "Left";
        case BlockFace::Right:
            return "Right";
        case BlockFace::Top:
            return "Top";
        case BlockFace::Bottom:
            return "Bottom";
        }
        return "Unknown";
    }

    // Holds the result of a successful raycast.
    struct RaycastResult
    {
        glm::ivec3 block_position;
        BlockFace face;
    };

    // Performs a raycast from the player's camera into the world.
    // Returns the hit block and face if a solid block is found within the max distance.
    std::optional<RaycastResult> perform_raycast(
        const flint::Camera &camera,
        const flint::Chunk &chunk,
        float max_distance);

} // namespace flint