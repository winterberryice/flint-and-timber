#include "player.h"
#include "world.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace flint
{
    namespace player
    {
        // Helper function to get AABBs of solid blocks near the player
        std::vector<physics::AABB> get_nearby_block_aabbs(const physics::AABB &player_world_box, const flint::World &world)
        {
            std::vector<physics::AABB> nearby_blocks;

            // Determine the range of world block coordinates that the player's AABB might overlap.
            int min_bx = static_cast<int>(std::floor(player_world_box.min.x - 1.0f));
            int max_bx = static_cast<int>(std::ceil(player_world_box.max.x + 1.0f));
            int min_by = static_cast<int>(std::floor(player_world_box.min.y - 1.0f));
            int max_by = static_cast<int>(std::ceil(player_world_box.max.y + 1.0f));
            int min_bz = static_cast<int>(std::floor(player_world_box.min.z - 1.0f));
            int max_bz = static_cast<int>(std::ceil(player_world_box.max.z + 1.0f));

            for (int bx = min_bx; bx < max_bx; ++bx)
            {
                for (int by = min_by; by < max_by; ++by)
                {
                    for (int bz = min_bz; bz < max_bz; ++bz)
                    {
                        const Block *block = world.getBlock(bx, by, bz);
                        if (block && block->isSolid())
                        {
                            glm::vec3 block_min_corner(static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bz));
                            glm::vec3 block_max_corner(bx + 1.0f, by + 1.0f, bz + 1.0f);
                            nearby_blocks.emplace_back(block_min_corner, block_max_corner);
                        }
                    }
                }
            }
            return nearby_blocks;
        }

        Player::Player(glm::vec3 initial_position, float initial_yaw, float initial_pitch, float sensitivity)
            : position(initial_position),
              velocity(0.0f),
              local_bounding_box(
                  glm::vec3(-physics::PLAYER_HALF_WIDTH, 0.0f, -physics::PLAYER_HALF_WIDTH),
                  glm::vec3(physics::PLAYER_HALF_WIDTH, physics::PLAYER_HEIGHT, physics::PLAYER_HALF_WIDTH)),
              on_ground(false),
              yaw(initial_yaw),
              pitch(initial_pitch),
              mouse_sensitivity(sensitivity)
        {
        }

        bool Player::on_mouse_click(const SDL_MouseButtonEvent &button, World &world)
        {
            if (m_block_action_cooldown > 0.0f)
            {
                return false;
            }

            auto selected_block_opt = get_selected_block();
            if (!selected_block_opt.has_value())
            {
                return false;
            }

            auto selected_block = selected_block_opt.value();

            if (button.button == SDL_BUTTON_LEFT) // Remove block
            {
                world.setBlock(
                    selected_block.block_position.x,
                    selected_block.block_position.y,
                    selected_block.block_position.z,
                    BlockType::Air);
                m_block_action_cooldown = BLOCK_ACTION_COOLDOWN_SECONDS;
                return true;
            }
            else if (button.button == SDL_BUTTON_RIGHT) // Place block
            {
                glm::ivec3 new_block_pos = selected_block.block_position + selected_block.face_normal;

                // Player collision check
                physics::AABB new_block_aabb(
                    glm::vec3(new_block_pos),
                    glm::vec3(new_block_pos) + glm::vec3(1.0f));

                if (!get_world_bounding_box().intersects(new_block_aabb))
                {
                    // Get the block type from the selected hotbar slot
                    auto selected_block_type = inventory.getSelectedBlockType();
                    if (selected_block_type.has_value())
                    {
                        world.setBlock(
                            new_block_pos.x,
                            new_block_pos.y,
                            new_block_pos.z,
                            selected_block_type.value());
                        m_block_action_cooldown = BLOCK_ACTION_COOLDOWN_SECONDS;
                        return true;
                    }
                }
            }

            return false;
        }

        void Player::handle_input(const SDL_Event &event)
        {
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
            {
                bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.key)
                {
                case SDLK_W:
                    movement_intention.forward = pressed;
                    break;
                case SDLK_S:
                    movement_intention.backward = pressed;
                    break;
                case SDLK_A:
                    movement_intention.left = pressed;
                    break;
                case SDLK_D:
                    movement_intention.right = pressed;
                    break;
                case SDLK_SPACE:
                    movement_intention.jump = pressed;
                    break;
                case SDLK_1:
                    if (pressed) inventory.selectHotbarSlot(0);
                    break;
                case SDLK_2:
                    if (pressed) inventory.selectHotbarSlot(1);
                    break;
                case SDLK_3:
                    if (pressed) inventory.selectHotbarSlot(2);
                    break;
                case SDLK_4:
                    if (pressed) inventory.selectHotbarSlot(3);
                    break;
                case SDLK_5:
                    if (pressed) inventory.selectHotbarSlot(4);
                    break;
                case SDLK_6:
                    if (pressed) inventory.selectHotbarSlot(5);
                    break;
                case SDLK_7:
                    if (pressed) inventory.selectHotbarSlot(6);
                    break;
                case SDLK_8:
                    if (pressed) inventory.selectHotbarSlot(7);
                    break;
                case SDLK_9:
                    if (pressed) inventory.selectHotbarSlot(8);
                    break;
                }
            }
        }

        void Player::process_mouse_movement(float delta_x, float delta_y)
        {
            yaw += delta_x * mouse_sensitivity;
            pitch -= delta_y * mouse_sensitivity; // Inverted

            // Clamp pitch
            pitch = std::clamp(pitch, -89.0f, 89.0f);
        }

        void Player::update(float dt, const flint::World &world)
        {
            // Update cooldown
            if (m_block_action_cooldown > 0.0f)
            {
                m_block_action_cooldown -= dt;
            }

            // Raycasting
            cast_ray(world);

            // 1. Apply Inputs & Intentions
            glm::vec3 intended_horizontal_velocity(0.0f);
            float yaw_radians = glm::radians(yaw);
            glm::vec3 horizontal_forward = glm::normalize(glm::vec3(cos(yaw_radians), 0.0f, sin(yaw_radians)));
            glm::vec3 horizontal_right = glm::normalize(glm::cross(horizontal_forward, glm::vec3(0, 1, 0)));

            if (movement_intention.forward)
                intended_horizontal_velocity += horizontal_forward;
            if (movement_intention.backward)
                intended_horizontal_velocity -= horizontal_forward;
            if (movement_intention.left)
                intended_horizontal_velocity -= horizontal_right;
            if (movement_intention.right)
                intended_horizontal_velocity += horizontal_right;

            if (glm::length2(intended_horizontal_velocity) > 0.0f)
            {
                intended_horizontal_velocity = glm::normalize(intended_horizontal_velocity) * physics::WALK_SPEED;
                velocity.x = intended_horizontal_velocity.x;
                velocity.z = intended_horizontal_velocity.z;
            }
            else
            {
                // Apply friction
                float speed = glm::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
                if (speed > 0.01f)
                {
                    float drop = speed * physics::FRICTION_COEFFICIENT * dt * 60.0f; // Frame-rate independent
                    float scale = std::max(speed - drop, 0.0f) / speed;
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
            velocity.y -= physics::GRAVITY * dt;

            // 3. Handle Jumping
            if (movement_intention.jump && on_ground)
            {
                velocity.y = physics::JUMP_FORCE;
                on_ground = false;
            }
            movement_intention.jump = false; // Consume jump intention

            // 4. Collision Detection and Resolution (Axis-by-Axis)
            glm::vec3 desired_move = velocity * dt;
            on_ground = false; // Reset before Y-axis collision check

            // --- Y-AXIS COLLISION ---
            position.y += desired_move.y;
            physics::AABB player_world_box = get_world_bounding_box();
            auto nearby_y_blocks = get_nearby_block_aabbs(player_world_box, world);

            for (const auto &block_box : nearby_y_blocks)
            {
                if (player_world_box.intersects(block_box))
                {
                    if (desired_move.y > 0.0f)
                    { // Moving up
                        position.y = block_box.min.y - local_bounding_box.max.y - 0.0001f;
                    }
                    else
                    { // Moving down
                        position.y = block_box.max.y - local_bounding_box.min.y + 0.0001f;
                        on_ground = true;
                    }
                    velocity.y = 0.0f;
                    desired_move.y = 0.0f;
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
                    { // Moving right
                        position.x = block_box.min.x - local_bounding_box.max.x - 0.0001f;
                    }
                    else if (desired_move.x < 0.0f)
                    { // Moving left
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
                    { // Moving "forward" in world +Z
                        position.z = block_box.min.z - local_bounding_box.max.z - 0.0001f;
                    }
                    else if (desired_move.z < 0.0f)
                    { // Moving "backward" in world +Z
                        position.z = block_box.max.z - local_bounding_box.min.z + 0.0001f;
                    }
                    velocity.z = 0.0f;
                    break;
                }
            }
        }

        glm::vec3 Player::get_position() const { return position; }
        float Player::get_yaw() const { return yaw; }
        float Player::get_pitch() const { return pitch; }

        std::optional<raycast::RaycastResult> Player::get_selected_block() const
        {
            return selected_block;
        }

        glm::vec3 Player::get_camera_position() const
        {
            // Position the camera at eye level
            return position + glm::vec3(0.0f, physics::PLAYER_EYE_HEIGHT, 0.0f);
        }

        glm::vec3 Player::get_camera_forward_vector() const
        {
            float yaw_rad = glm::radians(yaw);
            float pitch_rad = glm::radians(pitch);

            return glm::normalize(glm::vec3(
                cos(yaw_rad) * cos(pitch_rad),
                sin(pitch_rad),
                sin(yaw_rad) * cos(pitch_rad)));
        }

        void Player::cast_ray(const flint::World &world)
        {
            glm::vec3 camera_pos = get_camera_position();
            glm::vec3 forward = get_camera_forward_vector();
            selected_block = raycast::raycast(camera_pos, forward, 5.0f, world);
        }

        physics::AABB Player::get_world_bounding_box() const
        {
            return physics::AABB(position + local_bounding_box.min, position + local_bounding_box.max);
        }

        inventory::Inventory& Player::get_inventory()
        {
            return inventory;
        }

        const inventory::Inventory& Player::get_inventory() const
        {
            return inventory;
        }
    }
}
