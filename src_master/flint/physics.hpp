#pragma once

#include <glm/glm.hpp>

namespace flint
{
    namespace physics
    {
        // Physics Constants
        const float GRAVITY = 9.81f * 2.8f; // m/s^2, doubled for more "gamey" feel
        const float JUMP_FORCE = 8.0f;      // m/s
        const float WALK_SPEED = 4.0f;      // m/s
        const float FRICTION_COEFFICIENT = 0.8f; // Dimensionless, used for damping

        // Player Dimensions
        const float PLAYER_WIDTH = 0.6f;    // meters
        const float PLAYER_HEIGHT = 1.8f;   // meters
        const float PLAYER_EYE_HEIGHT = 1.6f; // meters, from feet
        const float PLAYER_HALF_WIDTH = PLAYER_WIDTH / 2.0f;

        struct AABB
        {
            glm::vec3 min;
            glm::vec3 max;

            AABB(const glm::vec3 &min, const glm::vec3 &max) : min(min), max(max) {}

            // Method to check if this AABB intersects with another one.
            bool intersects(const AABB &other) const
            {
                return (min.x < other.max.x && max.x > other.min.x) &&
                       (min.y < other.max.y && max.y > other.min.y) &&
                       (min.z < other.max.z && max.z > other.min.z);
            }
        };
    }
}
