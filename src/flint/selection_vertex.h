#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <cstddef> // Required for the offsetof macro

namespace flint
{
    struct SelectionVertex
    {
        glm::vec3 position;

        static inline WGPUVertexBufferLayout getLayout()
        {
            static WGPUVertexAttribute attributes[] = {
                // Attribute 0: Position
                {
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = offsetof(SelectionVertex, position),
                    .shaderLocation = 0,
                }};

            WGPUVertexBufferLayout layout{};
            layout.arrayStride = sizeof(SelectionVertex);
            layout.stepMode = WGPUVertexStepMode_Vertex;
            layout.attributeCount = sizeof(attributes) / sizeof(WGPUVertexAttribute);
            layout.attributes = attributes;

            return layout;
        }
    };

} // namespace flint