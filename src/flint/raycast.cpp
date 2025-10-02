#include "raycast.h"
#include "player.h"
#include "world.h"
#include "block.h"
#include "physics.h"
#include <cmath>
#include <limits>
#include <glm/glm.hpp>

namespace flint {

std::optional<RaycastResult> cast_ray(
    const player::Player& player,
    const World& world,
    float max_distance) {

    glm::vec3 eye_position = player.get_position() + glm::vec3(0.0f, physics::PLAYER_EYE_HEIGHT, 0.0f);

    // Convert degrees to radians for trig functions
    float yaw_rad = glm::radians(player.get_yaw());
    float pitch_rad = glm::radians(player.get_pitch());

    glm::vec3 ray_direction = glm::normalize(glm::vec3(
        cos(yaw_rad) * cos(pitch_rad),
        sin(pitch_rad),
        sin(yaw_rad) * cos(pitch_rad)
    ));

    glm::ivec3 current_voxel_coord(
        floor(eye_position.x),
        floor(eye_position.y),
        floor(eye_position.z)
    );

    int step_x = (ray_direction.x > 0.0f) ? 1 : -1;
    int step_y = (ray_direction.y > 0.0f) ? 1 : -1;
    int step_z = (ray_direction.z > 0.0f) ? 1 : -1;

    float t_delta_x = (std::abs(ray_direction.x) < 1e-6f) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.x);
    float t_delta_y = (std::abs(ray_direction.y) < 1e-6f) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.y);
    float t_delta_z = (std::abs(ray_direction.z) < 1e-6f) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.z);

    float t_max_x = (ray_direction.x > 0.0f)
        ? (current_voxel_coord.x + 1.0f - eye_position.x) / ray_direction.x
        : (eye_position.x - current_voxel_coord.x) / -ray_direction.x;
    float t_max_y = (ray_direction.y > 0.0f)
        ? (current_voxel_coord.y + 1.0f - eye_position.y) / ray_direction.y
        : (eye_position.y - current_voxel_coord.y) / -ray_direction.y;
    float t_max_z = (ray_direction.z > 0.0f)
        ? (current_voxel_coord.z + 1.0f - eye_position.z) / ray_direction.z
        : (eye_position.z - current_voxel_coord.z) / -ray_direction.z;

    float current_distance = 0.0f;
    BlockFace last_face;

    while (current_distance < max_distance) {
        if (t_max_x < t_max_y) {
            if (t_max_x < t_max_z) {
                current_distance = t_max_x;
                current_voxel_coord.x += step_x;
                t_max_x += t_delta_x;
                last_face = (step_x > 0) ? BlockFace::NegX : BlockFace::PosX;
            } else {
                current_distance = t_max_z;
                current_voxel_coord.z += step_z;
                t_max_z += t_delta_z;
                last_face = (step_z > 0) ? BlockFace::NegZ : BlockFace::PosZ;
            }
        } else {
            if (t_max_y < t_max_z) {
                current_distance = t_max_y;
                current_voxel_coord.y += step_y;
                t_max_y += t_delta_y;
                last_face = (step_y > 0) ? BlockFace::NegY : BlockFace::PosY;
            } else {
                current_distance = t_max_z;
                current_voxel_coord.z += step_z;
                t_max_z += t_delta_z;
                last_face = (step_z > 0) ? BlockFace::NegZ : BlockFace::PosZ;
            }
        }

        if (current_distance > max_distance) {
            return std::nullopt;
        }

        const Block* block = world.get_block_at_world(current_voxel_coord);
        if (block && block->type != BlockType::Air) {
            return RaycastResult{current_voxel_coord, last_face};
        }
    }

    return std::nullopt;
}

} // namespace flint