#pragma once

#include <glm/glm.hpp>
#include <optional>

#include "chunk.h"

namespace flint
{
    namespace raycast
    {
        struct RaycastResult
        {
            glm::ivec3 block_position;
            // Can add face information later if needed
        };

        std::optional<RaycastResult> raycast(
            const glm::vec3 &ray_origin,
            const glm::vec3 &ray_direction,
            float max_distance,
            const flint::Chunk &chunk);
    }
}