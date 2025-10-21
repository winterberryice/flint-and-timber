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
            glm::ivec3 face_normal;
        };

        std::optional<RaycastResult> raycast(
            const glm::vec3 &ray_origin,
            const glm::vec3 &ray_direction,
            float max_distance,
            const flint::Chunk &chunk);
    }
}