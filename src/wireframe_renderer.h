#pragma once

#include "world.h"
#include "raycast.h" // For BlockFace
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <webgpu/webgpu.h>
#include <vector>
#include <optional>

namespace flint
{

    struct WireframeVertex
    {
        glm::vec3 position;
    };

    struct ModelUniformData
    {
        glm::mat4 model_matrix;
    };

    class WireframeRenderer
    {
    public:
        WireframeRenderer(
            WGPUDevice device,
            WGPUTextureFormat surface_format,
            WGPUTextureFormat depth_format,
            WGPUBindGroupLayout camera_bind_group_layout);
        ~WireframeRenderer();

        void update_selection(std::optional<glm::ivec3> position);

        void draw(WGPURenderPassEncoder render_pass, WGPUQueue queue, const World &world);

        struct FaceRenderInfo
        {
            BlockFace face;
            uint32_t index_offset;
            uint32_t index_count;
        };

    private:
        WGPURenderPipeline render_pipeline = nullptr;
        WGPUBuffer vertex_buffer = nullptr;
        WGPUBuffer index_buffer = nullptr;

        std::vector<FaceRenderInfo> face_render_info;

        WGPUBuffer model_uniform_buffer = nullptr;
        WGPUBindGroup model_bind_group = nullptr;

        ModelUniformData model_data;
        std::optional<glm::ivec3> current_selected_block_pos;
    };

} // namespace flint
