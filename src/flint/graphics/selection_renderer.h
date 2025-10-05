#pragma once

#pragma once

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

#include "../camera.h"
#include "render_pipeline.h"
#include "selection_mesh.h"

namespace flint::graphics
{
    // Uniforms for the model matrix
    struct alignas(16) ModelUniform
    {
        glm::mat4 model;
    };

    class SelectionRenderer
    {
    public:
        SelectionRenderer();
        ~SelectionRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat);
        void create_mesh(WGPUDevice device);
        void render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera, const std::optional<glm::ivec3> &selected_block_pos, const std::vector<glm::ivec3> &exposed_faces);
        void cleanup();

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_cameraUniformBuffer = nullptr;
        CameraUniform m_cameraUniform;

        WGPUBuffer m_modelUniformBuffer = nullptr;
        ModelUniform m_modelUniform;

        SelectionMesh m_selectionMesh;

        bool is_visible = false;
    };

} // namespace flint::graphics