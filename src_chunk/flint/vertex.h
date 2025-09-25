#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <cstddef> // Required for the offsetof macro

namespace flint
{

    // This is the C++ equivalent of your Rust `pub struct Vertex`.
    // It uses `glm::vec3` for 3D vectors, which is the C++ replacement for glam's Vec3.
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;

        // This static function describes the memory layout of a single vertex to the GPU pipeline.
        // It's the C++ equivalent of the `desc()` method.
        // We use `inline` to allow the definition to exist in the header file without causing
        // linker errors if this header is included in multiple .cpp files.
        static inline WGPUVertexBufferLayout getLayout()
        {
            // We define the attributes in a static array. This is important because the
            // WGPUVertexBufferLayout struct stores a *pointer* to the attributes.
            // Making the array static ensures the pointer remains valid after this function returns.
            static WGPUVertexAttribute attributes[] = {
                // Attribute 0: Position
                // Corresponds to `@location(0)` in the WGSL vertex shader.
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x3,
                    /* .offset = */ offsetof(Vertex, position), // offsetof calculates the byte offset automatically
                    /* .shaderLocation = */ 0,
                },
                // Attribute 1: Color
                // Corresponds to `@location(1)` in the WGSL vertex shader.
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x3,
                    /* .offset = */ offsetof(Vertex, color),
                    /* .shaderLocation = */ 1,
                }};

            WGPUVertexBufferLayout layout{};
            // How many bytes to step forward in the buffer to find the next vertex.
            layout.arrayStride = sizeof(Vertex);
            // How often the data in this buffer is advanced. 'Vertex' means per-vertex.
            layout.stepMode = WGPUVertexStepMode_Vertex;
            // The list of attributes that make up a single vertex.
            layout.attributeCount = sizeof(attributes) / sizeof(WGPUVertexAttribute);
            layout.attributes = attributes;

            return layout;
        }
    };

} // namespace flint