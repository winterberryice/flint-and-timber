#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <SDL3/SDL_events.h>
#include <optional>

#include "physics.h"
#include "raycast.h"
#include "inventory.h"

namespace flint
{
    class World; // Forward declaration

    namespace player
    {
        // Input state for player movement intentions
        struct PlayerMovementIntention
        {
            bool forward = false;
            bool backward = false;
            bool left = false;
            bool right = false;
            bool jump = false;
        };

        class Player
        {
        public:
            Player(glm::vec3 initial_position, float initial_yaw, float initial_pitch, float mouse_sensitivity);

            void handle_input(const SDL_Event &event);
            void process_mouse_movement(float delta_x, float delta_y);
            void update(float dt, const flint::World &world);
            bool on_mouse_click(const SDL_MouseButtonEvent &button, World &world);

            glm::vec3 get_position() const;
            float get_yaw() const;
            float get_pitch() const;
            glm::vec3 get_camera_forward_vector() const;
            glm::vec3 get_camera_position() const;
            std::optional<raycast::RaycastResult> get_selected_block() const;
            physics::AABB get_world_bounding_box() const;
            inventory::Inventory& get_inventory();
            const inventory::Inventory& get_inventory() const;

        private:
            void cast_ray(const flint::World &world);

            glm::vec3 position; // Position of the player's feet
            glm::vec3 velocity;

            // Bounding box relative to the player's position
            physics::AABB local_bounding_box;

            bool on_ground;

            // Camera orientation
            float yaw;   // Radians
            float pitch; // Radians
            float mouse_sensitivity;

            PlayerMovementIntention movement_intention;

            std::optional<raycast::RaycastResult> selected_block;

            // Cooldown for block placement/removal to prevent single-press multi-actions
            const float BLOCK_ACTION_COOLDOWN_SECONDS = 0.2f; // 200ms
            float m_block_action_cooldown = 0.0f;

            // Inventory
            inventory::Inventory inventory;
        };
    }
}
