#pragma once

#include <webgpu/webgpu.h>
#include <cstring>

namespace flint::init
{

    inline WGPUStringView makeStringView(const char *str)
    {
        return WGPUStringView{
            .data = str,
            .length = str ? strlen(str) : 0};
    }

} // namespace flint::init