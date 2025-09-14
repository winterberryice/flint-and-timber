#pragma once

#include "glm/glm.hpp"
#include "physics.hpp"
#include <vector>

namespace flint {

    // Forward declaration
    class World;

    // Input state for player movement intentions
    struct PlayerMovementIntention {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool jump = false;
    };

    class Player {
    public:
        glm::vec3 position; // Position of the player's feet, centered horizontally
        glm::vec3 velocity;
        AABB local_bounding_box; // Relative to player's position (feet)
        bool on_ground;

        // Camera orientation fields
        float yaw;   // Radians. Rotation around the Y axis (vertical)
        float pitch; // Radians. Rotation around the X axis (horizontal)
        float mouse_sensitivity;

        // Movement intention state
        PlayerMovementIntention movement_intention;

        Player(const glm::vec3& initial_position, float initial_yaw, float initial_pitch, float sensitivity);

        void process_mouse_movement(double delta_x, double delta_y);

        void update_physics_and_collision(float dt, const World& world);

        AABB get_world_bounding_box() const;
    };

    // Helper function to get AABBs of solid blocks near the player
    std::vector<AABB> get_nearby_block_aabbs(const AABB& player_world_box, const World& world);


} // namespace flint
