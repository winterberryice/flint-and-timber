#include "buffer.h"

#include <iostream>
#include <stdexcept>
#include <cstring> // For memcpy

#include "utils.h"

namespace flint::init
{

    WGPUBuffer create_uniform_buffer(WGPUDevice device, const char *label, uint64_t size)
    {
        WGPUBufferDescriptor uniformBufferDesc = {};
        uniformBufferDesc.label = makeStringView(label);
        uniformBufferDesc.size = size;
        uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        uniformBufferDesc.mappedAtCreation = false;

        WGPUBuffer uniformBuffer = wgpuDeviceCreateBuffer(device, &uniformBufferDesc);
        if (!uniformBuffer)
        {
            std::cerr << "Failed to create uniform buffer: " << label << std::endl;
            throw std::runtime_error("Failed to create uniform buffer!");
        }

        return uniformBuffer;
    }

    WGPUBuffer create_vertex_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.label = makeStringView(label);
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bufferDesc.mappedAtCreation = true; // We will map the buffer to copy data into it.

        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        if (!buffer)
        {
            std::cerr << "Failed to create vertex buffer: " << label << std::endl;
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        // Get a pointer to the mapped memory and copy the data into it.
        void* mappedRange = wgpuBufferGetMappedRange(buffer, 0, size);
        memcpy(mappedRange, data, size);

        // Unmap the buffer before using it on the GPU.
        wgpuBufferUnmap(buffer);

        return buffer;
    }

    WGPUBuffer create_index_buffer(WGPUDevice device, const char *label, const void *data, uint64_t size)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.label = makeStringView(label);
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        bufferDesc.mappedAtCreation = true; // We will map the buffer to copy data into it.

        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        if (!buffer)
        {
            std::cerr << "Failed to create index buffer: " << label << std::endl;
            throw std::runtime_error("Failed to create index buffer!");
        }

        // Get a pointer to the mapped memory and copy the data into it.
        void* mappedRange = wgpuBufferGetMappedRange(buffer, 0, size);
        memcpy(mappedRange, data, size);

        // Unmap the buffer before using it on the GPU.
        wgpuBufferUnmap(buffer);

        return buffer;
    }

} // namespace flint::init