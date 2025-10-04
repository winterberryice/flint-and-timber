#include "raycast.h"
#include "chunk.h"
#include "camera.h"
#include <cmath>
#include <iostream>

namespace flint
{
    // Implementation of the Amanatides and Woo algorithm for voxel traversal.
    std::optional<RaycastResult> perform_raycast(
        const flint::Camera &camera,
        const flint::Chunk &chunk,
        float max_distance)
    {
        glm::vec3 ray_origin = camera.eye;
        glm::vec3 ray_direction = glm::normalize(camera.target - camera.eye);

        // The current voxel coordinates.
        glm::ivec3 current_voxel(std::floor(ray_origin.x), std::floor(ray_origin.y), std::floor(ray_origin.z));

        // The direction to step in for each axis (1, -1, or 0).
        glm::ivec3 step(
            (ray_direction.x > 0) ? 1 : -1,
            (ray_direction.y > 0) ? 1 : -1,
            (ray_direction.z > 0) ? 1 : -1);

        // The distance along the ray to the next voxel boundary for each axis.
        glm::vec3 next_boundary(
            (step.x > 0) ? (current_voxel.x + 1) : current_voxel.x,
            (step.y > 0) ? (current_voxel.y + 1) : current_voxel.y,
            (step.z > 0) ? (current_voxel.z + 1) : current_voxel.z);

        // The distance needed to travel along the ray to cross one voxel in each axis.
        glm::vec3 t_delta(
            std::abs(1.0f / ray_direction.x),
            std::abs(1.0f / ray_direction.y),
            std::abs(1.0f / ray_direction.z));

        // The current distance traveled along the ray for each axis.
        glm::vec3 t_max(
            (next_boundary.x - ray_origin.x) / ray_direction.x,
            (next_boundary.y - ray_origin.y) / ray_direction.y,
            (next_boundary.z - ray_origin.z) / ray_direction.z);

        float current_distance = 0.0f;
        BlockFace hit_face;

        BlockFace last_face; // Store the face of the *previous* step

        while (current_distance < max_distance)
        {
            // Check if the current voxel is solid.
            if (chunk.is_solid(current_voxel.x, current_voxel.y, current_voxel.z))
            {
                return RaycastResult{current_voxel, last_face};
            }

            // Advance to the next voxel boundary.
            if (t_max.x < t_max.y)
            {
                if (t_max.x < t_max.z)
                {
                    current_distance = t_max.x;
                    t_max.x += t_delta.x;
                    current_voxel.x += step.x;
                    last_face = (step.x > 0) ? BlockFace::Left : BlockFace::Right;
                }
                else
                {
                    current_distance = t_max.z;
                    t_max.z += t_delta.z;
                    current_voxel.z += step.z;
                    last_face = (step.z > 0) ? BlockFace::Front : BlockFace::Back;
                }
            }
            else
            {
                if (t_max.y < t_max.z)
                {
                    current_distance = t_max.y;
                    t_max.y += t_delta.y;
                    current_voxel.y += step.y;
                    last_face = (step.y > 0) ? BlockFace::Bottom : BlockFace::Top;
                }
                else
                {
                    current_distance = t_max.z;
                    t_max.z += t_delta.z;
                    current_voxel.z += step.z;
                    last_face = (step.z > 0) ? BlockFace::Front : BlockFace::Back;
                }
            }
        }

        // No block was hit within the maximum distance.
        return std::nullopt;
    }

} // namespace flint