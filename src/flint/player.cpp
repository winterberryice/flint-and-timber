#include "player.hpp"
#include "world.hpp" // Include the full definition of World
#include <cmath>
#include <vector>
#include "block.hpp"
#include <algorithm> // for std::max/min

#include "chunk.hpp"

namespace flint
{
    namespace
    {
        std::vector<AABB> get_nearby_block_aabbs(const AABB &player_world_box, const World &world)
        {
            std::vector<AABB> nearby_blocks;

            int min_world_block_x = static_cast<int>(std::floor(player_world_box.min.x - 1.0f));
            int max_world_block_x = static_cast<int>(std::ceil(player_world_box.max.x + 1.0f));
            int min_world_block_y = static_cast<int>(std::floor(player_world_box.min.y - 1.0f));
            int max_world_block_y = static_cast<int>(std::ceil(player_world_box.max.y + 1.0f));
            int min_world_block_z = static_cast<int>(std::floor(player_world_box.min.z - 1.0f));
            int max_world_block_z = static_cast<int>(std::ceil(player_world_box.max.z + 1.0f));

            for (int world_bx = min_world_block_x; world_bx < max_world_block_x; ++world_bx)
            {
                for (int world_by = min_world_block_y; world_by < max_world_block_y; ++world_by)
                {
                    for (int world_bz = min_world_block_z; world_bz < max_world_block_z; ++world_bz)
                    {
                        if (world_by < 0 || world_by >= CHUNK_HEIGHT)
                        {
                            continue;
                        }

                        auto block_opt = world.get_block_at_world(
                            static_cast<float>(world_bx),
                            static_cast<float>(world_by),
                            static_cast<float>(world_bz));

                        if (block_opt && block_opt->is_solid())
                        {
                            glm::vec3 block_min_corner(world_bx, world_by, world_bz);
                            glm::vec3 block_max_corner(world_bx + 1.0f, world_by + 1.0f, world_bz + 1.0f);
                            nearby_blocks.emplace_back(block_min_corner, block_max_corner);
                        }
                    }
                }
            }
            return nearby_blocks;
        }
    }

    Player::Player(glm::vec3 initial_position, float initial_yaw, float initial_pitch, float sensitivity)
        : position(initial_position),
          velocity(0.0f),
          local_bounding_box(
              AABB(
                  glm::vec3(-Physics::PLAYER_HALF_WIDTH, 0.0f, -Physics::PLAYER_HALF_WIDTH),
                  glm::vec3(Physics::PLAYER_HALF_WIDTH, Physics::PLAYER_HEIGHT, Physics::PLAYER_HALF_WIDTH))),
          on_ground(false),
          yaw(initial_yaw),
          pitch(initial_pitch),
          mouse_sensitivity(sensitivity) {}

    void Player::process_mouse_movement(double delta_x, double delta_y)
    {
        yaw += static_cast<float>(delta_x) * mouse_sensitivity;
        pitch -= static_cast<float>(delta_y) * mouse_sensitivity;

        const float max_pitch = glm::radians(89.0f);
        const float min_pitch = -max_pitch;
        pitch = std::max(min_pitch, std::min(pitch, max_pitch));
    }

    void Player::update_physics_and_collision(float dt, const World &world)
    {
        // 1. Apply Inputs & Intentions
        glm::vec3 intended_horizontal_velocity(0.0f);
        glm::vec3 horizontal_forward(std::cos(yaw), 0.0f, std::sin(yaw));
        glm::vec3 horizontal_right(horizontal_forward.z, 0.0f, -horizontal_forward.x);

        if (movement_intention.forward)
        {
            intended_horizontal_velocity += horizontal_forward;
        }
        if (movement_intention.backward)
        {
            intended_horizontal_velocity -= horizontal_forward;
        }
        if (movement_intention.left)
        {
            intended_horizontal_velocity -= horizontal_right;
        }
        if (movement_intention.right)
        {
            intended_horizontal_velocity += horizontal_right;
        }

        if (glm::length2(intended_horizontal_velocity) > 0.0f)
        {
            intended_horizontal_velocity = glm::normalize(intended_horizontal_velocity) * Physics::WALK_SPEED;
            velocity.x = intended_horizontal_velocity.x;
            velocity.z = intended_horizontal_velocity.z;
        }
        else
        {
            float speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
            if (speed > 0.01f)
            {
                float drop = speed * Physics::FRICTION_COEFFICIENT * dt;
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
        velocity.y -= Physics::GRAVITY * dt;

        // 3. Handle Jumping
        if (movement_intention.jump && on_ground)
        {
            velocity.y = Physics::JUMP_FORCE;
            on_ground = false;
        }
        movement_intention.jump = false;

        // 4. Collision Detection and Resolution
        glm::vec3 desired_move = velocity * dt;
        on_ground = false;

        // Y-AXIS
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

        // X-AXIS
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

        // Z-AXIS
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
} // namespace flint
