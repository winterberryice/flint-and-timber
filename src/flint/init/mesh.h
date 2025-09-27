#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPUBuffer create_triangle_vertex_buffer(WGPUDevice device, WGPUQueue queue);

} // namespace flint::init