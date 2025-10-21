#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <SDL3/SDL_events.h>
#include <optional>

#include "physics.h"
#include "raycast.h"

namespace flint
{
    class Chunk; // Forward declaration

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
            void update(float dt, const flint::Chunk &chunk);

            glm::vec3 get_position() const;
            float get_yaw() const;
            float get_pitch() const;
            glm::vec3 get_camera_forward_vector() const;
            glm::vec3 get_camera_position() const;
            std::optional<raycast::RaycastResult> get_selected_block() const;
            physics::AABB get_world_bounding_box() const;

        private:
            void cast_ray(const flint::Chunk &chunk);

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
        };
    }
}
