#include "player.h"

#include "world.h" // TODO: This file does not exist yet.
#include "chunk.h" // TODO: This file does not exist yet for CHUNK_HEIGHT.
#include "glm/gtc/constants.hpp"
#include <cmath>
#include <algorithm>

namespace flint
{

    Player::Player(const glm::vec3 &initial_position, float initial_yaw, float initial_pitch, float sensitivity)
        : position(initial_position),
          velocity(0.0f),
          local_bounding_box(
              glm::vec3(-PLAYER_HALF_WIDTH, 0.0f, -PLAYER_HALF_WIDTH),
              glm::vec3(PLAYER_HALF_WIDTH, PLAYER_HEIGHT, PLAYER_HALF_WIDTH)),
          on_ground(false),
          yaw(initial_yaw),
          pitch(initial_pitch),
          mouse_sensitivity(sensitivity),
          movement_intention() {}

    void Player::process_mouse_movement(double delta_x, double delta_y)
    {
        float dx = static_cast<float>(delta_x) * mouse_sensitivity;
        float dy = static_cast<float>(delta_y) * mouse_sensitivity;

        yaw += dx;
        pitch -= dy; // Inverted

        const float max_pitch = glm::radians(89.0f);
        const float min_pitch = -max_pitch;
        pitch = std::clamp(pitch, min_pitch, max_pitch);

        // Optional: Normalize yaw
        // yaw = fmod(yaw, 2.0f * glm::pi<float>());
    }

    void Player::update_physics_and_collision(float dt, const World &world)
    {
        // 1. Apply Inputs & Intentions
        glm::vec3 intended_horizontal_velocity(0.0f);

        glm::vec3 horizontal_forward(cos(yaw), 0.0f, sin(yaw));
        if (glm::dot(horizontal_forward, horizontal_forward) > 0.0f)
        {
            horizontal_forward = glm::normalize(horizontal_forward);
        }

        if (movement_intention.forward)
            intended_horizontal_velocity += horizontal_forward;
        if (movement_intention.backward)
            intended_horizontal_velocity -= horizontal_forward;

        glm::vec3 horizontal_right(horizontal_forward.z, 0.0f, -horizontal_forward.x);

        if (movement_intention.left)
            intended_horizontal_velocity += horizontal_right;
        if (movement_intention.right)
            intended_horizontal_velocity -= horizontal_right;

        if (glm::dot(intended_horizontal_velocity, intended_horizontal_velocity) > 0.0f)
        {
            intended_horizontal_velocity = glm::normalize(intended_horizontal_velocity) * WALK_SPEED;
            velocity.x = intended_horizontal_velocity.x;
            velocity.z = intended_horizontal_velocity.z;
        }
        else
        {
            // Apply friction
            float speed = sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
            if (speed > 0.01f)
            {
                float drop = speed * FRICTION_COEFFICIENT * dt;
                float scale = std::max(0.0f, speed - drop) / speed;
                velocity.x *= scale;
                velocity.z *= scale;
            }
            else
            {
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            }
        }

        // 2. Apply Gravity
        velocity.y -= GRAVITY * dt;

        // 3. Handle Jumping
        if (movement_intention.jump && on_ground)
        {
            velocity.y = JUMP_FORCE;
            on_ground = false;
        }
        movement_intention.jump = false;

        // 4. Collision Detection and Resolution
        glm::vec3 desired_move = velocity * dt;
        on_ground = false;

        // --- Y-AXIS COLLISION ---
        position.y += desired_move.y;
        AABB player_world_box = get_world_bounding_box();
        auto nearby_y_blocks = get_nearby_block_aabbs(player_world_box, world);
        for (const auto &block_box : nearby_y_blocks)
        {
            if (player_world_box.intersects(block_box))
            {
                if (desired_move.y > 0.0f)
                {
                    position.y = block_box.min.y - local_bounding_box.max.y - 0.0001f;
                }
                else
                {
                    position.y = block_box.max.y - local_bounding_box.min.y + 0.0001f;
                    on_ground = true;
                }
                velocity.y = 0.0f;
                break;
            }
        }

        // --- X-AXIS COLLISION ---
        position.x += desired_move.x;
        player_world_box = get_world_bounding_box();
        auto nearby_x_blocks = get_nearby_block_aabbs(player_world_box, world);
        for (const auto &block_box : nearby_x_blocks)
        {
            if (player_world_box.intersects(block_box))
            {
                if (desired_move.x > 0.0f)
                {
                    position.x = block_box.min.x - local_bounding_box.max.x - 0.0001f;
                }
                else
                {
                    position.x = block_box.max.x - local_bounding_box.min.x + 0.0001f;
                }
                velocity.x = 0.0f;
                break;
            }
        }

        // --- Z-AXIS COLLISION ---
        position.z += desired_move.z;
        player_world_box = get_world_bounding_box();
        auto nearby_z_blocks = get_nearby_block_aabbs(player_world_box, world);
        for (const auto &block_box : nearby_z_blocks)
        {
            if (player_world_box.intersects(block_box))
            {
                if (desired_move.z > 0.0f)
                {
                    position.z = block_box.min.z - local_bounding_box.max.z - 0.0001f;
                }
                else
                {
                    position.z = block_box.max.z - local_bounding_box.min.z + 0.0001f;
                }
                velocity.z = 0.0f;
                break;
            }
        }
    }

    AABB Player::get_world_bounding_box() const
    {
        return AABB(position + local_bounding_box.min, position + local_bounding_box.max);
    }

    std::vector<AABB> get_nearby_block_aabbs(const AABB &player_world_box, const World &world)
    {
        std::vector<AABB> nearby_blocks;

        int32_t min_x = static_cast<int32_t>(floor(player_world_box.min.x - 1.0f));
        int32_t max_x = static_cast<int32_t>(ceil(player_world_box.max.x + 1.0f));
        int32_t min_y = static_cast<int32_t>(floor(player_world_box.min.y - 1.0f));
        int32_t max_y = static_cast<int32_t>(ceil(player_world_box.max.y + 1.0f));
        int32_t min_z = static_cast<int32_t>(floor(player_world_box.min.z - 1.0f));
        int32_t max_z = static_cast<int32_t>(ceil(player_world_box.max.z + 1.0f));

        for (int32_t bx = min_x; bx < max_x; ++bx)
        {
            for (int32_t by = min_y; by < max_y; ++by)
            {
                for (int32_t bz = min_z; bz < max_z; ++bz)
                {
                    // TODO: CHUNK_HEIGHT is defined in chunk.rs, needs to be in chunk.h
                    if (by < 0 || by >= CHUNK_HEIGHT)
                    {
                        continue;
                    }

                    auto block_opt = world.get_block_at_world(static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bz));
                    if (block_opt && block_opt->is_solid())
                    {
                        glm::vec3 min_corner(bx, by, bz);
                        glm::vec3 max_corner(bx + 1.0f, by + 1.0f, bz + 1.0f);
                        nearby_blocks.emplace_back(min_corner, max_corner);
                    }
                }
            }
        }
        return nearby_blocks;
    }

} // namespace flint
