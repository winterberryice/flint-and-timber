#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPUBuffer create_uniform_buffer(WGPUDevice device, const char *label, uint64_t size);

} // namespace flint::init