#pragma once

#include "world.h"
#include <queue>
#include <glm/glm.hpp>

namespace flint
{
    class Light
    {
    public:
        static void calculate_sky_light(World *world);
        static void propagate_light_addition(World *world, int x, int y, int z);
        static void propagate_light_removal(World *world, int x, int y, int z, uint8_t light_level);

    private:
        static void run_light_propagation_queue(World *world, std::queue<glm::ivec3> &queue);
    };

} // namespace flint