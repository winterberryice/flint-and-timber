#pragma once

#include "chunk.h"
#include <vector>
#include <queue>
#include <tuple>

namespace flint
{
    class Light
    {
    public:
        static void calculate_sky_light(Chunk *chunk);
    };

} // namespace flint