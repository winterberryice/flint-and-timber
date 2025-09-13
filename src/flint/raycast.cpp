#include "raycast.hpp"
#include <cmath>
#include <limits>

namespace flint
{
    std::optional<std::pair<glm::ivec3, BlockFace>> cast_ray(
        const Player &player,
        const World &world,
        float max_distance)
    {

        glm::vec3 eye_position = player.position + glm::vec3(0.0f, Physics::PLAYER_EYE_HEIGHT, 0.0f);
        glm::vec3 ray_direction = glm::normalize(glm::vec3(
            std::cos(player.yaw) * std::cos(player.pitch),
            std::sin(player.pitch),
            std::sin(player.yaw) * std::cos(player.pitch)));

        glm::ivec3 current_voxel_coord(
            static_cast<int>(std::floor(eye_position.x)),
            static_cast<int>(std::floor(eye_position.y)),
            static_cast<int>(std::floor(eye_position.z)));

        int step_x = (ray_direction.x > 0.0f) ? 1 : -1;
        int step_y = (ray_direction.y > 0.0f) ? 1 : -1;
        int step_z = (ray_direction.z > 0.0f) ? 1 : -1;

        float t_delta_x = (std::abs(ray_direction.x) < 1e-6) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.x);
        float t_delta_y = (std::abs(ray_direction.y) < 1e-6) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.y);
        float t_delta_z = (std::abs(ray_direction.z) < 1e-6) ? std::numeric_limits<float>::max() : std::abs(1.0f / ray_direction.z);

        float t_max_x = (ray_direction.x > 0.0f) ? (static_cast<float>(current_voxel_coord.x) + 1.0f - eye_position.x) / ray_direction.x : (eye_position.x - static_cast<float>(current_voxel_coord.x)) / -ray_direction.x;
        float t_max_y = (ray_direction.y > 0.0f) ? (static_cast<float>(current_voxel_coord.y) + 1.0f - eye_position.y) / ray_direction.y : (eye_position.y - static_cast<float>(current_voxel_coord.y)) / -ray_direction.y;
        float t_max_z = (ray_direction.z > 0.0f) ? (static_cast<float>(current_voxel_coord.z) + 1.0f - eye_position.z) / ray_direction.z : (eye_position.z - static_cast<float>(current_voxel_coord.z)) / -ray_direction.z;

        if (std::isnan(t_max_x) || t_max_x < 0.0f)
            t_max_x = t_delta_x;
        if (std::isnan(t_max_y) || t_max_y < 0.0f)
            t_max_y = t_delta_y;
        if (std::isnan(t_max_z) || t_max_z < 0.0f)
            t_max_z = t_delta_z;

        float current_distance = 0.0f;
        BlockFace last_face;

        while (current_distance < max_distance)
        {
            if (t_max_x < t_max_y)
            {
                if (t_max_x < t_max_z)
                {
                    current_distance = t_max_x;
                    current_voxel_coord.x += step_x;
                    t_max_x += t_delta_x;
                    last_face = (step_x > 0) ? BlockFace::NegX : BlockFace::PosX;
                }
                else
                {
                    current_distance = t_max_z;
                    current_voxel_coord.z += step_z;
                    t_max_z += t_delta_z;
                    last_face = (step_z > 0) ? BlockFace::NegZ : BlockFace::PosZ;
                }
            }
            else
            {
                if (t_max_y < t_max_z)
                {
                    current_distance = t_max_y;
                    current_voxel_coord.y += step_y;
                    t_max_y += t_delta_y;
                    last_face = (step_y > 0) ? BlockFace::NegY : BlockFace::PosY;
                }
                else
                {
                    current_distance = t_max_z;
                    current_voxel_coord.z += step_z;
                    t_max_z += t_delta_z;
                    last_face = (step_z > 0) ? BlockFace::NegZ : BlockFace::PosZ;
                }
            }

            if (current_distance > max_distance)
            {
                return std::nullopt;
            }

            auto block_opt = world.get_block_at_world(static_cast<float>(current_voxel_coord.x), static_cast<float>(current_voxel_coord.y), static_cast<float>(current_voxel_coord.z));
            if (block_opt && block_opt->block_type != BlockType::Air)
            {
                return std::make_pair(current_voxel_coord, last_face);
            }
        }

        return std::nullopt;
    }
} // namespace flint
