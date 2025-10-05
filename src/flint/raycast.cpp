#include "raycast.h"
#include <cmath>
#include <algorithm>

namespace flint
{
    namespace raycast
    {
        std::optional<RaycastResult> raycast(
            const glm::vec3 &ray_origin,
            const glm::vec3 &ray_direction,
            float max_distance,
            const flint::Chunk &chunk)
        {
            // Initial block coordinates
            glm::ivec3 current_block(std::floor(ray_origin.x), std::floor(ray_origin.y), std::floor(ray_origin.z));

            // Ray step direction (1, -1, or 0)
            glm::ivec3 step(
                (ray_direction.x > 0) ? 1 : ((ray_direction.x < 0) ? -1 : 0),
                (ray_direction.y > 0) ? 1 : ((ray_direction.y < 0) ? -1 : 0),
                (ray_direction.z > 0) ? 1 : ((ray_direction.z < 0) ? -1 : 0));

            // Calculate initial tMax values
            glm::vec3 next_boundary(
                current_block.x + (step.x > 0 ? 1 : 0),
                current_block.y + (step.y > 0 ? 1 : 0),
                current_block.z + (step.z > 0 ? 1 : 0));

            glm::vec3 tMax(
                (step.x != 0) ? (next_boundary.x - ray_origin.x) / ray_direction.x : std::numeric_limits<float>::max(),
                (step.y != 0) ? (next_boundary.y - ray_origin.y) / ray_direction.y : std::numeric_limits<float>::max(),
                (step.z != 0) ? (next_boundary.z - ray_origin.z) / ray_direction.z : std::numeric_limits<float>::max());

            // Calculate tDelta values
            glm::vec3 tDelta(
                (step.x != 0) ? std::abs(1.0f / ray_direction.x) : std::numeric_limits<float>::max(),
                (step.y != 0) ? std::abs(1.0f / ray_direction.y) : std::numeric_limits<float>::max(),
                (step.z != 0) ? std::abs(1.0f / ray_direction.z) : std::numeric_limits<float>::max());

            float distance = 0.0f;

            while (distance < max_distance)
            {
                if (chunk.is_solid(current_block.x, current_block.y, current_block.z))
                {
                    return RaycastResult{current_block};
                }

                // Advance to the next block
                if (tMax.x < tMax.y)
                {
                    if (tMax.x < tMax.z)
                    {
                        current_block.x += step.x;
                        distance = tMax.x;
                        tMax.x += tDelta.x;
                    }
                    else
                    {
                        current_block.z += step.z;
                        distance = tMax.z;
                        tMax.z += tDelta.z;
                    }
                }
                else
                {
                    if (tMax.y < tMax.z)
                    {
                        current_block.y += step.y;
                        distance = tMax.y;
                        tMax.y += tDelta.y;
                    }
                    else
                    {
                        current_block.z += step.z;
                        distance = tMax.z;
                        tMax.z += tDelta.z;
                    }
                }
            }

            return std::nullopt; // No block found within max_distance
        }
    }
}