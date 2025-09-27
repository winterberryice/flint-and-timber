#include "mesh.h"

#include <iostream>
#include <stdexcept>

#include "utils.h"

namespace flint::init
{

    WGPUBuffer create_triangle_vertex_buffer(WGPUDevice device, WGPUQueue queue)
    {
        std::cout << "Creating triangle vertex data..." << std::endl;

        // Triangle vertices in NDC coordinates (x, y, z)
        float triangleVertices[] = {
            0.0f, 0.5f, 0.0f,   // Top vertex
            -0.5f, -0.5f, 0.0f, // Bottom left
            0.5f, -0.5f, 0.0f   // Bottom right
        };

        // Create vertex buffer descriptor
        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.nextInChain = nullptr;
        vertexBufferDesc.label = makeStringView("Triangle Vertex Buffer");
        vertexBufferDesc.size = sizeof(triangleVertices);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.mappedAtCreation = false;

        // Create the vertex buffer
        WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
        if (!vertexBuffer)
        {
            std::cerr << "Failed to create vertex buffer!" << std::endl;
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        // Upload vertex data to GPU
        wgpuQueueWriteBuffer(queue, vertexBuffer, 0, triangleVertices, sizeof(triangleVertices));

        std::cout << "Vertex buffer created and data uploaded" << std::endl;

        return vertexBuffer;
    }

} // namespace flint::init