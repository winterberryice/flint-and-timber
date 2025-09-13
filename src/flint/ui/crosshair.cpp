#include "crosshair.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace flint::ui
{
    // Helper to read shader file
    std::string read_shader_file(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    struct CrosshairVertex
    {
        glm::vec2 position;
    };

    Crosshair::Crosshair() {}

    Crosshair::~Crosshair()
    {
        cleanup();
    }

    void Crosshair::cleanup()
    {
        if (m_projection_bind_group) wgpuBindGroupRelease(m_projection_bind_group);
        m_projection_bind_group = nullptr;
        if (m_projection_bind_group_layout) wgpuBindGroupLayoutRelease(m_projection_bind_group_layout);
        m_projection_bind_group_layout = nullptr;
        if (m_projection_buffer) wgpuBufferRelease(m_projection_buffer);
        m_projection_buffer = nullptr;
        if (m_render_pipeline) wgpuRenderPipelineRelease(m_render_pipeline);
        m_render_pipeline = nullptr;
        if (m_vertex_buffer) wgpuBufferRelease(m_vertex_buffer);
        m_vertex_buffer = nullptr;
    }

    bool Crosshair::initialize(WGPUDevice device, WGPUTextureFormat surface_format, uint32_t width, uint32_t height)
    {
        float half_length = 10.0f;
        float thickness = 1.0f;

        std::vector<CrosshairVertex> vertices = {
            // Horizontal bar
            {{-half_length, -thickness}}, {{half_length, -thickness}}, {{-half_length, thickness}},
            {{half_length, -thickness}}, {{half_length, thickness}}, {{-half_length, thickness}},
            // Vertical bar
            {{-thickness, -half_length}}, {{thickness, -half_length}}, {{-thickness, half_length}},
            {{thickness, -half_length}}, {{thickness, half_length}}, {{-thickness, half_length}},
        };
        m_num_vertices = static_cast<uint32_t>(vertices.size());

        WGPUBufferDescriptor vertex_buffer_desc = {};
        vertex_buffer_desc.size = vertices.size() * sizeof(CrosshairVertex);
        vertex_buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        m_vertex_buffer = wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_vertex_buffer, 0, vertices.data(), vertex_buffer_desc.size);

        // Projection matrix
        resize(width, height, wgpuDeviceGetQueue(device));

        WGPUBufferDescriptor proj_buffer_desc = {};
        proj_buffer_desc.size = sizeof(glm::mat4);
        proj_buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        m_projection_buffer = wgpuDeviceCreateBuffer(device, &proj_buffer_desc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_projection_buffer, 0, &m_projection_matrix, sizeof(glm::mat4));

        WGPUBindGroupLayoutEntry bgl_entry = {};
        bgl_entry.binding = 0;
        bgl_entry.visibility = WGPUShaderStage_Vertex;
        bgl_entry.buffer.type = WGPUBufferBindingType_Uniform;
        WGPUBindGroupLayoutDescriptor bgl_desc = {};
        bgl_desc.entryCount = 1;
        bgl_desc.entries = &bgl_entry;
        m_projection_bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bgl_desc);

        WGPUBindGroupEntry bg_entry = {};
        bg_entry.binding = 0;
        bg_entry.buffer = m_projection_buffer;
        bg_entry.size = sizeof(glm::mat4);
        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout = m_projection_bind_group_layout;
        bg_desc.entryCount = 1;
        bg_desc.entries = &bg_entry;
        m_projection_bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);

        // Shader
        std::string shader_source = read_shader_file("assets/shaders/crosshair.wgsl");
        WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
        wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSL;
        wgsl_desc.code = shader_source.c_str();
        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = &wgsl_desc.chain;
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

        // Pipeline
        WGPUPipelineLayoutDescriptor pipeline_layout_desc = {};
        pipeline_layout_desc.bindGroupLayoutCount = 1;
        pipeline_layout_desc.bindGroupLayouts = &m_projection_bind_group_layout;
        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_desc);

        WGPUVertexAttribute vert_attr = {};
        vert_attr.format = WGPUVertexFormat_Float32x2;
        vert_attr.offset = 0;
        vert_attr.shaderLocation = 0;
        WGPUVertexBufferLayout vb_layout = {};
        vb_layout.arrayStride = sizeof(CrosshairVertex);
        vb_layout.attributeCount = 1;
        vb_layout.attributes = &vert_attr;

        WGPUColorTargetState color_target = {};
        color_target.format = surface_format;
        color_target.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragment_state = {};
        fragment_state.module = shader_module;
        fragment_state.entryPoint = "fs_main";
        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target;

        WGPURenderPipelineDescriptor pipeline_desc = {};
        pipeline_desc.layout = pipeline_layout;
        pipeline_desc.vertex.module = shader_module;
        pipeline_desc.vertex.entryPoint = "vs_main";
        pipeline_desc.vertex.bufferCount = 1;
        pipeline_desc.vertex.buffers = &vb_layout;
        pipeline_desc.fragment = &fragment_state;
        pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipeline_desc.primitive.cullMode = WGPUCullMode_None;

        m_render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

        wgpuPipelineLayoutRelease(pipeline_layout);
        wgpuShaderModuleRelease(shader_module);

        return m_render_pipeline != nullptr;
    }

    void Crosshair::render(WGPURenderPassEncoder render_pass) const
    {
        if (!m_render_pipeline) return;
        wgpuRenderPassEncoderSetPipeline(render_pass, m_render_pipeline);
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, m_projection_bind_group, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, m_vertex_buffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDraw(render_pass, m_num_vertices, 1, 0, 0);
    }

    void Crosshair::resize(uint32_t width, uint32_t height, WGPUQueue queue)
    {
        if (width > 0 && height > 0)
        {
            m_projection_matrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
            if(m_projection_buffer) {
                 wgpuQueueWriteBuffer(queue, m_projection_buffer, 0, &m_projection_matrix, sizeof(glm::mat4));
            }
        }
    }
} // namespace flint::ui
