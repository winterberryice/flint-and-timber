#pragma once

#include <glm/glm.hpp>

namespace flint
{
    namespace Physics
    {
        const float GRAVITY = 9.81f * 2.8f;
        const float JUMP_FORCE = 8.0f;
        const float WALK_SPEED = 4.0f;
        const float FRICTION_COEFFICIENT = 0.8f;

        const float PLAYER_WIDTH = 0.6f;
        const float PLAYER_HEIGHT = 1.8f;
        const float PLAYER_EYE_HEIGHT = 1.6f;
        const float PLAYER_HALF_WIDTH = PLAYER_WIDTH / 2.0f;
    }

    struct AABB
    {
        glm::vec3 min;
        glm::vec3 max;

        AABB(glm::vec3 min_v, glm::vec3 max_v) : min(min_v), max(max_v) {}

        bool intersects(const AABB &other) const
        {
            return (min.x < other.max.x && max.x > other.min.x) &&
                   (min.y < other.max.y && max.y > other.min.y) &&
                   (min.z < other.max.z && max.z > other.min.z);
        }
    };
} // namespace flint
