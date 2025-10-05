#include "buffer.h"

#include <iostream>
#include <cstring>

namespace flint::init
{

    WGPUBuffer create_buffer(WGPUDevice device, const char *label, uint64_t size, WGPUBufferUsageFlags usage)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = usage;
        bufferDesc.mappedAtCreation = false;
        return wgpuDeviceCreateBuffer(device, &bufferDesc);
    }

    WGPUBuffer create_vertex_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_Vertex;
        bufferDesc.mappedAtCreation = true;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        memcpy(wgpuBufferGetMappedRange(buffer, 0, size), data, size);
        wgpuBufferUnmap(buffer);
        return buffer;
    }

    WGPUBuffer create_index_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_Index;
        bufferDesc.mappedAtCreation = true;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        memcpy(wgpuBufferGetMappedRange(buffer, 0, size), data, size);
        wgpuBufferUnmap(buffer);
        return buffer;
    }

    WGPUBuffer create_uniform_buffer(WGPUDevice device, const char *label, uint64_t size)
    {
        return create_buffer(device, label, size, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    }

} // namespace flint::init