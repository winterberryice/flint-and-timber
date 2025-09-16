#pragma once

#include <webgpu/webgpu.h>
#include <string.h>

struct flint_utils
{
    static WGPUStringView makeStringView(const char *str)
    {
        return WGPUStringView{
            .data = str,
            .length = str ? strlen(str) : 0};
    }
};
