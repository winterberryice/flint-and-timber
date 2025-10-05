#pragma once

#include <webgpu/webgpu.h>
#include <cstdint>

namespace flint::init
{
    WGPUBuffer create_buffer(WGPUDevice device, const char *label, uint64_t size, WGPUBufferUsageFlags usage);

    WGPUBuffer create_vertex_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size);

    WGPUBuffer create_index_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size);

    WGPUBuffer create_uniform_buffer(WGPUDevice device, const char *label, uint64_t size);

} // namespace flint::init