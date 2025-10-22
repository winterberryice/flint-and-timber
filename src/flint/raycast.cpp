#include "raycast.h"
#include "world.h"
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
            const flint::World &world)
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
            glm::ivec3 face_normal(0);

            while (distance < max_distance)
            {
                const Block *block = world.getBlock(current_block.x, current_block.y, current_block.z);
                if (block && block->isSolid())
                {
                    return RaycastResult{current_block, face_normal};
                }

                // Advance to the next block
                if (tMax.x < tMax.y && tMax.x < tMax.z)
                {
                    distance = tMax.x;
                    tMax.x += tDelta.x;
                    current_block.x += step.x;
                    face_normal = glm::ivec3(-step.x, 0, 0);
                }
                else if (tMax.y < tMax.z)
                {
                    distance = tMax.y;
                    tMax.y += tDelta.y;
                    current_block.y += step.y;
                    face_normal = glm::ivec3(0, -step.y, 0);
                }
                else
                {
                    distance = tMax.z;
                    tMax.z += tDelta.z;
                    current_block.z += step.z;
                    face_normal = glm::ivec3(0, 0, -step.z);
                }
            }

            return std::nullopt; // No block found within max_distance
        }
    }
}