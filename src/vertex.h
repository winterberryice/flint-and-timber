#pragma once

#include <webgpu/webgpu.h>
#include <vector>
#include <glm/glm.hpp>

namespace flint
{

    // This was defined in main.rs
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
        uint32_t tree_id;
        uint32_t sky_light;

        static WGPUVertexBufferLayout get_layout()
        {
            static std::vector<WGPUVertexAttribute> attributes{
                {nullptr, WGPUVertexFormat_Float32x3, offsetof(Vertex, position), 0},
                {nullptr, WGPUVertexFormat_Float32x3, offsetof(Vertex, color), 1},
                {nullptr, WGPUVertexFormat_Float32x2, offsetof(Vertex, uv), 2},
                {nullptr, WGPUVertexFormat_Uint32, offsetof(Vertex, tree_id), 3},
                {nullptr, WGPUVertexFormat_Uint32, offsetof(Vertex, sky_light), 4},
            };

            WGPUVertexBufferLayout layout = {};
            layout.arrayStride = sizeof(Vertex);
            layout.stepMode = WGPUVertexStepMode_Vertex;
            layout.attributeCount = static_cast<uint32_t>(attributes.size());
            layout.attributes = attributes.data();
            return layout;
        }
    };
}