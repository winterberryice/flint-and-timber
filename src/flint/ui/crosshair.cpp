#include "crosshair.h"

#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "../init/buffer.h"
#include "../init/pipeline.h"
#include "../init/shader.h"
#include "../init/utils.h"
#include "crosshair.wgsl.h"

namespace flint::ui
{
    namespace
    {
        struct CrosshairVertex
        {
            float position[2];
        };
    }

    Crosshair::Crosshair() = default;

    Crosshair::~Crosshair()
    {
        cleanup();
    }

    void Crosshair::init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surface_format, uint32_t width, uint32_t height)
    {
        // Define vertices for the crosshair (+) shape
        float half_length = 10.0f;
        float thickness = 1.0f;

        std::vector<CrosshairVertex> vertices = {
            // Horizontal bar
            {{-half_length, -thickness}},
            {{half_length, -thickness}},
            {{-half_length, thickness}},
            {{half_length, -thickness}},
            {{half_length, thickness}},
            {{-half_length, thickness}},

            // Vertical bar
            {{-thickness, -half_length}},
            {{thickness, -half_length}},
            {{-thickness, half_length}},
            {{thickness, -half_length}},
            {{thickness, half_length}},
            {{-thickness, half_length}},
        };

        m_num_vertices = static_cast<uint32_t>(vertices.size());

        m_vertex_buffer = init::create_buffer(
            device,
            "Crosshair Vertex Buffer",
            vertices.size() * sizeof(CrosshairVertex),
            WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        wgpuQueueWriteBuffer(queue, m_vertex_buffer, 0, vertices.data(), vertices.size() * sizeof(CrosshairVertex));

        // Create projection matrix and buffer
        m_projection_matrix = glm::ortho(
            -(static_cast<float>(width) / 2.0f),  // left
            (static_cast<float>(width) / 2.0f),   // right
            (static_cast<float>(height) / 2.0f),  // bottom
            -(static_cast<float>(height) / 2.0f), // top
            -1.0f,                               // near
            1.0f                                 // far
        );

        m_projection_buffer = init::create_buffer(
            device,
            "Crosshair Projection Buffer",
            sizeof(glm::mat4),
            WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        wgpuQueueWriteBuffer(queue, m_projection_buffer, 0, &m_projection_matrix, sizeof(glm::mat4));

        // Create bind group layout and bind group
        WGPUBindGroupLayoutEntry projection_bind_group_layout_entry{};
        projection_bind_group_layout_entry.binding = 0;
        projection_bind_group_layout_entry.visibility = WGPUShaderStage_Vertex;
        projection_bind_group_layout_entry.buffer.type = WGPUBufferBindingType_Uniform;

        WGPUBindGroupLayoutDescriptor projection_bind_group_layout_desc{};
        projection_bind_group_layout_desc.entryCount = 1;
        projection_bind_group_layout_desc.entries = &projection_bind_group_layout_entry;
        projection_bind_group_layout_desc.label = init::makeStringView("Crosshair Projection Bind Group Layout");
        WGPUBindGroupLayout projection_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &projection_bind_group_layout_desc);

        WGPUBindGroupEntry projection_bind_group_entry{};
        projection_bind_group_entry.binding = 0;
        projection_bind_group_entry.buffer = m_projection_buffer;
        projection_bind_group_entry.size = sizeof(glm::mat4);

        WGPUBindGroupDescriptor projection_bind_group_desc{};
        projection_bind_group_desc.layout = projection_bind_group_layout;
        projection_bind_group_desc.entryCount = 1;
        projection_bind_group_desc.entries = &projection_bind_group_entry;
        projection_bind_group_desc.label = init::makeStringView("Crosshair Projection Bind Group");
        m_projection_bind_group = wgpuDeviceCreateBindGroup(device, &projection_bind_group_desc);

        // Create shader module
        WGPUShaderModuleWGSLDescriptor shader_wgsl_desc{};
        shader_wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        shader_wgsl_desc.code = init::makeStringView(crosshair_shader_wgsl.data());

        WGPUShaderModuleDescriptor shader_desc{};
        shader_desc.nextInChain = &shader_wgsl_desc.chain;
        shader_desc.label = init::makeStringView("Crosshair Shader");
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);


        // Create render pipeline
        WGPUPipelineLayoutDescriptor pipeline_layout_desc{};
        pipeline_layout_desc.bindGroupLayoutCount = 1;
        pipeline_layout_desc.bindGroupLayouts = &projection_bind_group_layout;
        pipeline_layout_desc.label = init::makeStringView("Crosshair Pipeline Layout");
        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_desc);

        WGPUVertexAttribute vertex_attribute{};
        vertex_attribute.format = WGPUVertexFormat_Float32x2;
        vertex_attribute.offset = 0;
        vertex_attribute.shaderLocation = 0;

        WGPUVertexBufferLayout vertex_buffer_layout{};
        vertex_buffer_layout.arrayStride = sizeof(CrosshairVertex);
        vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
        vertex_buffer_layout.attributeCount = 1;
        vertex_buffer_layout.attributes = &vertex_attribute;

        WGPUBlendState blend_state{};
        blend_state.color.operation = WGPUBlendOperation_Add;
        blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blend_state.alpha.operation = WGPUBlendOperation_Add;
        blend_state.alpha.srcFactor = WGPUBlendFactor_One;
        blend_state.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

        WGPUColorTargetState color_target_state{};
        color_target_state.format = surface_format;
        color_target_state.blend = &blend_state;
        color_target_state.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragment_state{};
        fragment_state.module = shader_module;
        fragment_state.entryPoint = init::makeStringView("fs_main");
        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target_state;

        WGPURenderPipelineDescriptor pipeline_desc{};
        pipeline_desc.label = init::makeStringView("Crosshair Render Pipeline");
        pipeline_desc.layout = pipeline_layout;
        pipeline_desc.vertex.module = shader_module;
        pipeline_desc.vertex.entryPoint = init::makeStringView("vs_main");
        pipeline_desc.vertex.bufferCount = 1;
        pipeline_desc.vertex.buffers = &vertex_buffer_layout;
        pipeline_desc.fragment = &fragment_state;
        pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
        pipeline_desc.primitive.cullMode = WGPUCullMode_None;
        pipeline_desc.depthStencil = nullptr;
        pipeline_desc.multisample.count = 1;
        pipeline_desc.multisample.mask = 0xFFFFFFFF;

        m_render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

        // Release temporary resources
        wgpuBindGroupLayoutRelease(projection_bind_group_layout);
        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuShaderModuleRelease(shader_module);
    }

    void Crosshair::resize(uint32_t width, uint32_t height, WGPUQueue queue)
    {
        if (width > 0 && height > 0)
        {
            m_projection_matrix = glm::ortho(
                -(static_cast<float>(width) / 2.0f),
                (static_cast<float>(width) / 2.0f),
                (static_cast<float>(height) / 2.0f),
                -(static_cast<float>(height) / 2.0f),
                -1.0f,
                1.0f);
            wgpuQueueWriteBuffer(queue, m_projection_buffer, 0, &m_projection_matrix, sizeof(glm::mat4));
        }
    }

    void Crosshair::draw(WGPURenderPassEncoder render_pass) const
    {
        if (!m_render_pipeline)
            return;
        wgpuRenderPassEncoderSetPipeline(render_pass, m_render_pipeline);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, m_projection_bind_group, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, m_vertex_buffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDraw(render_pass, m_num_vertices, 1, 0, 0);
    }

    void Crosshair::cleanup()
    {
        if (m_vertex_buffer)
        {
            wgpuBufferDestroy(m_vertex_buffer);
            wgpuBufferRelease(m_vertex_buffer);
            m_vertex_buffer = nullptr;
        }
        if (m_projection_buffer)
        {
            wgpuBufferDestroy(m_projection_buffer);
            wgpuBufferRelease(m_projection_buffer);
            m_projection_buffer = nullptr;
        }
        if (m_projection_bind_group)
        {
            wgpuBindGroupRelease(m_projection_bind_group);
            m_projection_bind_group = nullptr;
        }
        if (m_render_pipeline)
        {
            wgpuRenderPipelineRelease(m_render_pipeline);
            m_render_pipeline = nullptr;
        }
    }

} // namespace flint::ui