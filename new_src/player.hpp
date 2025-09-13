#pragma once

#include <glm/glm.hpp>
#include "physics.hpp"

// Forward declaration
class World;

struct PlayerMovementIntention {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool jump = false;
};

class Player {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    AABB local_bounding_box;
    bool on_ground;

    float yaw;
    float pitch;
    float mouse_sensitivity;

    PlayerMovementIntention movement_intention;

    Player(glm::vec3 initial_position, float initial_yaw, float initial_pitch, float sensitivity);

    void process_mouse_movement(double delta_x, double delta_y);
    void update_physics_and_collision(float dt, const World& world);
    AABB get_world_bounding_box() const;
};
