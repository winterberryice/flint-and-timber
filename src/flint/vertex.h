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
        glm::vec2 uv; // New UV coordinates

        static inline WGPUVertexBufferLayout getLayout()
        {
            static WGPUVertexAttribute attributes[] = {
                // Attribute 0: Position
                {
                    .nextInChain = nullptr,
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = offsetof(Vertex, position),
                    .shaderLocation = 0,
                },
                // Attribute 1: Color
                {
                    .nextInChain = nullptr,
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = offsetof(Vertex, color),
                    .shaderLocation = 1,
                },
                // Attribute 2: UV
                {
                    .nextInChain = nullptr,
                    .format = WGPUVertexFormat_Float32x2,
                    .offset = offsetof(Vertex, uv),
                    .shaderLocation = 2,
                }};

            WGPUVertexBufferLayout layout{};
            layout.arrayStride = sizeof(Vertex);
            layout.stepMode = WGPUVertexStepMode_Vertex;
            layout.attributeCount = sizeof(attributes) / sizeof(WGPUVertexAttribute);
            layout.attributes = attributes;

            return layout;
        }
    };

} // namespace flint