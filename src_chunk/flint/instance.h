#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <cstddef> // Required for the offsetof macro

namespace flint
{

    // This struct holds the per-instance data that we'll send to the GPU.
    // `alignas(16)` ensures that each instance in a buffer starts on a 16-byte boundary,
    // which is a common alignment requirement for performance on GPUs.
    struct alignas(16) InstanceRaw
    {
        // A glm::mat4 is laid out in memory as four consecutive vec4s, which is
        // exactly what the GPU expects for a mat4x4<f32>.
        glm::mat4 model_matrix;

        // The instance's color. glm::vec3 is 12 bytes.
        glm::vec3 color;

        // Explicit padding to ensure the total size of the struct is a multiple of 16 bytes.
        // (mat4=64 bytes) + (vec3=12 bytes) + (float=4 bytes) = 80 bytes. 80 is a multiple of 16.
        // This prevents alignment issues when we have an array of these instances in a buffer.
        float _padding;

        // Constructor, equivalent to `InstanceRaw::new`.
        explicit InstanceRaw(const glm::mat4 &matrix, const glm::vec3 &inst_color)
            : model_matrix(matrix), color(inst_color), _padding(0.0f) {}

        // Static function to describe the instance data layout to the GPU pipeline.
        // This is the C++ equivalent of `desc()`.
        static inline WGPUVertexBufferLayout getLayout()
        {
            // This static array holds the attribute descriptions.
            // It must be static so that the pointer to it remains valid after the function returns.
            static WGPUVertexAttribute attributes[] = {
                // The model matrix is passed to the shader as four separate vec4 attributes.
                // We assume shader locations 0-4 are used by the vertex position, normal, etc.
                // and start the instance attributes at location 5.

                // Column 0 of the matrix
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x4,
                    /* .offset = */ offsetof(InstanceRaw, model_matrix),
                    /* .shaderLocation = */ 5,
                },
                // Column 1
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x4,
                    /* .offset = */ offsetof(InstanceRaw, model_matrix) + sizeof(glm::vec4) * 1,
                    /* .shaderLocation = */ 6,
                },
                // Column 2
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x4,
                    /* .offset = */ offsetof(InstanceRaw, model_matrix) + sizeof(glm::vec4) * 2,
                    /* .shaderLocation = */ 7,
                },
                // Column 3
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x4,
                    /* .offset = */ offsetof(InstanceRaw, model_matrix) + sizeof(glm::vec4) * 3,
                    /* .shaderLocation = */ 8,
                },
                // Color attribute
                {
                    nullptr,
                    /* .format = */ WGPUVertexFormat_Float32x3,
                    /* .offset = */ offsetof(InstanceRaw, color),
                    /* .shaderLocation = */ 9,
                }};

            WGPUVertexBufferLayout layout{};
            // The stride is the total size of one instance's data.
            layout.arrayStride = sizeof(InstanceRaw);
            // The step mode is 'Instance', meaning the GPU will advance to the next
            // instance's data for each new instance it draws, not for each vertex.
            layout.stepMode = WGPUVertexStepMode_Instance;
            // The number of attributes describing this instance data.
            layout.attributeCount = sizeof(attributes) / sizeof(WGPUVertexAttribute);
            layout.attributes = attributes;

            return layout;
        }
    };

} // namespace flint