#include "buffer.h"

#include <iostream>
#include <stdexcept>

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

} // namespace flint::init