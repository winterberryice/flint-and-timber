#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <optional>

#include "../camera.h"
#include "../raycast.h"
#include "../world.h"

namespace flint::graphics {

class SelectionRenderer {
public:
    SelectionRenderer(WGPUDevice device, WGPUTextureFormat surface_format, const WGPUBindGroupLayout& camera_bind_group_layout);
    ~SelectionRenderer();

    void draw(
        WGPURenderPassEncoder render_pass,
        WGPUQueue queue,
        const World& world,
        const std::optional<RaycastResult>& selection);

private:
    void create_buffers();
    void create_pipeline();
    void create_bind_group();

    WGPUDevice device;
    WGPUTextureFormat surface_format;
    const WGPUBindGroupLayout& camera_bind_group_layout;

    WGPURenderPipeline render_pipeline = nullptr;
    WGPUBuffer vertex_buffer = nullptr;
    WGPUBuffer index_buffer = nullptr;
    uint32_t index_count = 0;

    WGPUBuffer model_uniform_buffer = nullptr;
    WGPUBindGroup model_bind_group = nullptr;
    WGPUBindGroupLayout model_bind_group_layout = nullptr;
};

} // namespace flint::graphics