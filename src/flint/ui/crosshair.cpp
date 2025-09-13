#include "crosshair.hpp"
#include <vector>
#include "glm/gtc/matrix_transform.hpp"
#include <string>

namespace flint::ui
{
    namespace
    {
        const char *shader_source = R"(
        struct Uniforms {
            projection: mat4x4<f32>,
        }

        @group(0) @binding(0) var<uniform> uniforms: Uniforms;

        @vertex
        fn vs_main(@location(0) in_pos: vec2<f32>) -> @builtin(position) vec4<f32> {
            return uniforms.projection * vec4<f32>(in_pos, 0.0, 1.0);
        }

        @fragment
        fn fs_main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 1.0, 1.0, 1.0); // White crosshair
        }
    )";
    }

    void Crosshair::initialize(wgpu::Device device, wgpu::TextureFormat texture_format, uint32_t width, uint32_t height)
    {
        const float half_length = 10.0f;
        const float thickness = 1.0f;

        const std::vector<glm::vec2> vertices = {
            // Horizontal bar
            {-half_length, -thickness},
            {half_length, -thickness},
            {-half_length, thickness},
            {half_length, -thickness},
            {half_length, thickness},
            {-half_length, thickness},
            // Vertical bar
            {-thickness, -half_length},
            {thickness, -half_length},
            {-thickness, half_length},
            {thickness, -half_length},
            {thickness, half_length},
            {-thickness, half_length},
        };
        num_vertices = static_cast<uint32_t>(vertices.size());

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = vertices.size() * sizeof(glm::vec2);
        bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
        vertex_buffer = device.createBuffer(&bufferDesc);
        device.getQueue().writeBuffer(vertex_buffer, 0, vertices.data(), bufferDesc.size);

        wgpu::ShaderModuleWGSLDescriptor shader_wgsl_desc;
        shader_wgsl_desc.code = shader_source;
        wgpu::ShaderModuleDescriptor shader_desc;
        shader_desc.nextInChain = &shader_wgsl_desc;
        wgpu::ShaderModule shader_module = device.createShaderModule(&shader_desc);

        wgpu::RenderPipelineDescriptor pipeline_desc;
        pipeline_desc.vertex.module = shader_module;
        pipeline_desc.vertex.entryPoint = "vs_main";
        wgpu::VertexBufferLayout vb_layout;
        vb_layout.arrayStride = sizeof(glm::vec2);
        vb_layout.stepMode = wgpu::VertexStepMode::Vertex;
        wgpu::VertexAttribute vert_attr;
        vert_attr.format = wgpu::VertexFormat::Float32x2;
        vert_attr.offset = 0;
        vert_attr.shaderLocation = 0;
        vb_layout.attributeCount = 1;
        vb_layout.attributes = &vert_attr;
        pipeline_desc.vertex.bufferCount = 1;
        pipeline_desc.vertex.buffers = &vb_layout;

        wgpu::ColorTargetState color_target;
        color_target.format = texture_format;
        wgpu::FragmentState frag_state;
        frag_state.module = shader_module;
        frag_state.entryPoint = "fs_main";
        frag_state.targetCount = 1;
        frag_state.targets = &color_target;
        pipeline_desc.fragment = &frag_state;

        pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

        render_pipeline = device.createRenderPipeline(&pipeline_desc);

        projection_buffer = device.createBuffer(
            wgpu::BufferDescriptor{
                .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
                .size = sizeof(glm::mat4)});

        wgpu::BindGroupDescriptor bg_desc;
        wgpu::BindGroupEntry bg_entry;
        bg_entry.binding = 0;
        bg_entry.buffer = projection_buffer;
        bg_desc.layout = render_pipeline.getBindGroupLayout(0);
        bg_desc.entryCount = 1;
        bg_desc.entries = &bg_entry;
        projection_bind_group = device.createBindGroup(&bg_desc);

        resize(width, height, device.getQueue());
    }

    void Crosshair::resize(uint32_t width, uint32_t height, wgpu::Queue queue)
    {
        if (width > 0 && height > 0)
        {
            projection_matrix = glm::ortho(
                -(width / 2.0f), static_cast<float>(width) / 2.0f,
                static_cast<float>(height) / 2.0f, -(static_cast<float>(height) / 2.0f),
                -1.0f, 1.0f);
            queue.writeBuffer(projection_buffer, 0, &projection_matrix, sizeof(glm::mat4));
        }
    }

    void Crosshair::render(wgpu::RenderPassEncoder render_pass, uint32_t width, uint32_t height)
    {
        render_pass.setPipeline(render_pipeline);
        render_pass.setBindGroup(0, projection_bind_group, 0, nullptr);
        render_pass.setVertexBuffer(0, vertex_buffer);
        render_pass.draw(num_vertices, 1, 0, 0);
    }
} // namespace flint::ui
