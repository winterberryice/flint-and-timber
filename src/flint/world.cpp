#include "world.hpp"
#include <cmath>
#include <algorithm>
#include <deque>
#include "vertex.hpp"
#include "cube_geometry.hpp"
#include "texture.hpp"

namespace flint
{
    namespace
    {
        const char *shader_source = R"(
            struct CameraUniform {
                view_proj: mat4x4<f32>,
            }
            @group(0) @binding(0)
            var<uniform> camera: CameraUniform;

            struct VertexInput {
                @location(0) position: vec3<f32>,
                @location(1) color: vec3<f32>,
                @location(2) uv: vec2<f32>,
                @location(3) tree_id: u32,
                @location(4) sky_light: u32,
            };

            struct VertexOutput {
                @builtin(position) clip_position: vec4<f32>,
                @location(0) original_color: vec3<f32>,
                @location(1) tex_coords: vec2<f32>,
                @location(2) tree_id: u32,
                @location(3) @interpolate(flat) sky_light: u32,
            };

            @vertex
            fn vs_main(model: VertexInput) -> VertexOutput {
                var out: VertexOutput;
                out.clip_position = camera.view_proj * vec4<f32>(model.position, 1.0);
                out.original_color = model.color;
                out.tex_coords = model.uv;
                out.tree_id = model.tree_id;
                out.sky_light = model.sky_light;
                return out;
            }

            @group(1) @binding(0) var t_diffuse: texture_2d<f32>;
            @group(1) @binding(1) var s_sampler: sampler;

            @fragment
            fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
                let sampled_color = textureSample(t_diffuse, s_sampler, in.tex_coords);
                if (sampled_color.a < 0.1) {
                    discard;
                }
                let light_intensity = f32(in.sky_light) / 15.0;
                return vec4<f32>(sampled_color.rgb * light_intensity, 1.0);
            }
        )";
    }

    World::World() {}

    void World::initialize(wgpu::Device device, wgpu::Queue queue)
    {
        wgpu::ShaderModuleWGSLDescriptor shader_wgsl_desc;
        shader_wgsl_desc.code = shader_source;
        wgpu::ShaderModuleDescriptor shader_desc;
        shader_desc.nextInChain = &shader_wgsl_desc;
        wgpu::ShaderModule shader_module = device.createShaderModule(&shader_desc);

        wgpu::BindGroupLayoutEntry bgl_entry_camera = {};
        bgl_entry_camera.binding = 0;
        bgl_entry_camera.visibility = wgpu::ShaderStage::Vertex;
        bgl_entry_camera.buffer.type = wgpu::BufferBindingType::Uniform;

        wgpu::BindGroupLayoutDescriptor bgl_desc_camera;
        bgl_desc_camera.entryCount = 1;
        bgl_desc_camera.entries = &bgl_entry_camera;
        wgpu::BindGroupLayout bgl_camera = device.createBindGroupLayout(&bgl_desc_camera);

        wgpu::BindGroupLayoutEntry bgl_entries_texture[2] = {};
        bgl_entries_texture[0].binding = 0;
        bgl_entries_texture[0].visibility = wgpu::ShaderStage::Fragment;
        bgl_entries_texture[0].texture.sampleType = wgpu::TextureSampleType::Float;
        bgl_entries_texture[1].binding = 1;
        bgl_entries_texture[1].visibility = wgpu::ShaderStage::Fragment;
        bgl_entries_texture[1].sampler.type = wgpu::SamplerBindingType::Filtering;

        wgpu::BindGroupLayoutDescriptor bgl_desc_texture;
        bgl_desc_texture.entryCount = 2;
        bgl_desc_texture.entries = bgl_entries_texture;
        wgpu::BindGroupLayout bgl_texture = device.createBindGroupLayout(&bgl_desc_texture);

        wgpu::PipelineLayoutDescriptor pl_desc;
        std::vector<wgpu::BindGroupLayout> bgls = {bgl_camera, bgl_texture};
        pl_desc.bindGroupLayoutCount = bgls.size();
        pl_desc.bindGroupLayouts = bgls.data();
        wgpu::PipelineLayout pipeline_layout = device.createPipelineLayout(&pl_desc);

        wgpu::RenderPipelineDescriptor pipeline_desc;
        pipeline_desc.layout = pipeline_layout;
        pipeline_desc.vertex.module = shader_module;
        pipeline_desc.vertex.entryPoint = "vs_main";

        wgpu::VertexAttribute vert_attrs[5];
        vert_attrs[0].format = wgpu::VertexFormat::Float32x3;
        vert_attrs[0].offset = offsetof(Vertex, position);
        vert_attrs[0].shaderLocation = 0;
        vert_attrs[1].format = wgpu::VertexFormat::Float32x3;
        vert_attrs[1].offset = offsetof(Vertex, color);
        vert_attrs[1].shaderLocation = 1;
        vert_attrs[2].format = wgpu::VertexFormat::Float32x2;
        vert_attrs[2].offset = offsetof(Vertex, uv);
        vert_attrs[2].shaderLocation = 2;
        vert_attrs[3].format = wgpu::VertexFormat::Uint32;
        vert_attrs[3].offset = offsetof(Vertex, tree_id);
        vert_attrs[3].shaderLocation = 3;
        vert_attrs[4].format = wgpu::VertexFormat::Uint32;
        vert_attrs[4].offset = offsetof(Vertex, sky_light);
        vert_attrs[4].shaderLocation = 4;

        wgpu::VertexBufferLayout vb_layout;
        vb_layout.arrayStride = sizeof(Vertex);
        vb_layout.attributeCount = 5;
        vb_layout.attributes = vert_attrs;
        pipeline_desc.vertex.bufferCount = 1;
        pipeline_desc.vertex.buffers = &vb_layout;

        wgpu::ColorTargetState color_target;
        color_target.format = wgpu::TextureFormat::BGRA8UnormSrgb; // This should be passed in
        wgpu::FragmentState frag_state;
        frag_state.module = shader_module;
        frag_state.entryPoint = "fs_main";
        frag_state.targetCount = 1;
        frag_state.targets = &color_target;
        pipeline_desc.fragment = &frag_state;

        pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipeline_desc.primitive.cullMode = wgpu::CullMode::Back;

        pipeline = device.createRenderPipeline(&pipeline_desc);

        // TODO: Load texture from "assets/textures/atlas.png"
        // Texture atlas_texture = Texture::load_from_memory(device, queue, ...);
        // wgpu::BindGroupEntry bg_entries[2];
        // bg_entries[0].binding = 0;
        // bg_entries[0].textureView = atlas_texture.view;
        // bg_entries[1].binding = 1;
        // bg_entries[1].sampler = atlas_texture.sampler;
        // wgpu::BindGroupDescriptor bg_desc;
        // bg_desc.layout = bgl_texture;
        // bg_desc.entryCount = 2;
        // bg_desc.entries = bg_entries;
        // bind_group = device.createBindGroup(&bg_desc);
    }

    void World::render(wgpu::RenderPassEncoder pass, wgpu::Buffer camera_buffer)
    {
        pass.setPipeline(pipeline);
        // pass.setBindGroup(1, bind_group);

        for (auto &pair : chunks)
        {
            Chunk &chunk = pair.second;
            // TODO: generate chunk mesh and render it
        }
    }

    Chunk *World::get_or_create_chunk(int chunk_x, int chunk_z)
    {
        auto it = chunks.find({chunk_x, chunk_z});
        if (it == chunks.end())
        {
            it = chunks.emplace(std::piecewise_construct,
                                std::forward_as_tuple(chunk_x, chunk_z),
                                std::forward_as_tuple(chunk_x, chunk_z))
                     .first;
            it->second.generate_terrain();
            it->second.calculate_sky_light();
        }
        return &it->second;
    }

    const Chunk *World::get_chunk(int chunk_x, int chunk_z) const
    {
        auto it = chunks.find({chunk_x, chunk_z});
        if (it != chunks.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::pair<std::pair<int, int>, std::tuple<int, int, int>> World::world_to_chunk_coords(float world_x, float world_y, float world_z)
    {
        int chunk_x = static_cast<int>(std::floor(world_x / CHUNK_WIDTH));
        int chunk_z = static_cast<int>(std::floor(world_z / CHUNK_DEPTH));
        int local_x = static_cast<int>(std::fmod(std::fmod(world_x, static_cast<float>(CHUNK_WIDTH)) + static_cast<float>(CHUNK_WIDTH), static_cast<float>(CHUNK_WIDTH)));
        int local_z = static_cast<int>(std::fmod(std::fmod(world_z, static_cast<float>(CHUNK_DEPTH)) + static_cast<float>(CHUNK_DEPTH), static_cast<float>(CHUNK_DEPTH)));
        int clamped_y = static_cast<int>(std::max(0.0f, std::min(world_y, static_cast<float>(CHUNK_HEIGHT - 1))));
        return {{chunk_x, chunk_z}, {local_x, clamped_y, local_z}};
    }

    std::optional<Block> World::get_block_at_world(float world_x, float world_y, float world_z) const
    {
        auto [chunk_coords, local_coords] = world_to_chunk_coords(world_x, world_y, world_z);
        auto [local_x, local_y, local_z] = local_coords;
        if (local_y >= CHUNK_HEIGHT)
        {
            return std::nullopt;
        }
        const Chunk *chunk = get_chunk(chunk_coords.first, chunk_coords.second);
        if (chunk)
        {
            return chunk->get_block(local_x, local_y, local_z);
        }
        return std::nullopt;
    }

    uint8_t World::get_light_level(glm::ivec3 pos) const
    {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
        {
            return 0;
        }
        auto block = get_block_at_world(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
        return block ? block->sky_light : 0;
    }

    void World::set_light_level(glm::ivec3 pos, uint8_t level)
    {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
        {
            return;
        }
        auto [chunk_coords, local_coords] = world_to_chunk_coords(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
        auto [local_x, local_y, local_z] = local_coords;

        auto it = chunks.find({chunk_coords.first, chunk_coords.second});
        if (it != chunks.end())
        {
            it->second.set_block_light(local_x, local_y, local_z, level);
        }
    }

    bool World::is_block_transparent(glm::ivec3 pos) const
    {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
        {
            return true;
        }
        auto block = get_block_at_world(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
        return block ? block->is_transparent() : true;
    }

    void World::propagate_light_addition(glm::ivec3 new_air_block_pos)
    {
        uint8_t max_light_from_neighbors = 0;
        std::deque<glm::ivec3> light_propagation_queue;

        const std::array<glm::ivec3, 6> neighbors = {
            glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
            glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
            glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)};

        for (const auto &offset : neighbors)
        {
            glm::ivec3 neighbor_pos = new_air_block_pos + offset;
            if (!is_block_transparent(neighbor_pos))
            {
                continue;
            }
            uint8_t neighbor_light = get_light_level(neighbor_pos);
            if (neighbor_light == 0)
            {
                continue;
            }
            uint8_t potential_light = (offset.y == 1 && neighbor_light == 15) ? 15 : neighbor_light - 1;
            if (potential_light > max_light_from_neighbors)
            {
                max_light_from_neighbors = potential_light;
            }
        }

        set_light_level(new_air_block_pos, max_light_from_neighbors);
        if (max_light_from_neighbors > 0)
        {
            light_propagation_queue.push_back(new_air_block_pos);
        }
        run_light_propagation_queue(light_propagation_queue);
    }

    void World::propagate_light_removal(glm::ivec3 new_solid_block_pos, uint8_t light_level_removed)
    {
        if (light_level_removed == 0)
            return;

        set_light_level(new_solid_block_pos, 0);

        std::deque<std::pair<glm::ivec3, uint8_t>> removal_queue;
        std::deque<glm::ivec3> relight_queue;
        removal_queue.push_back({new_solid_block_pos, light_level_removed});

        const std::array<glm::ivec3, 6> neighbors = {
            glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
            glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
            glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)};

        while (!removal_queue.empty())
        {
            auto [pos, light_level] = removal_queue.front();
            removal_queue.pop_front();

            for (const auto &offset : neighbors)
            {
                glm::ivec3 neighbor_pos = pos + offset;
                uint8_t neighbor_light = get_light_level(neighbor_pos);

                if (neighbor_light == 0)
                    continue;

                if (neighbor_light < light_level)
                {
                    set_light_level(neighbor_pos, 0);
                    removal_queue.push_back({neighbor_pos, neighbor_light});
                }
                else
                {
                    if (should_be_relit(neighbor_pos))
                    {
                        relight_queue.push_back(neighbor_pos);
                    }
                    else
                    {
                        set_light_level(neighbor_pos, 0);
                        removal_queue.push_back({neighbor_pos, neighbor_light});
                    }
                }
            }
        }
        run_light_propagation_queue(relight_queue);
    }

    bool World::should_be_relit(glm::ivec3 pos) const
    {
        const std::array<glm::ivec3, 6> neighbors = {
            glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
            glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
            glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)};
        for (const auto &offset : neighbors)
        {
            glm::ivec3 neighbor = pos + offset;
            uint8_t neighbor_light = get_light_level(neighbor);
            if (offset.y == 1 && neighbor_light == 15)
            {
                return true;
            }
            if (neighbor_light > get_light_level(pos))
            {
                return true;
            }
        }
        return false;
    }

    void World::run_light_propagation_queue(std::deque<glm::ivec3> &queue)
    {
        const std::array<glm::ivec3, 6> neighbors = {
            glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
            glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
            glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)};

        while (!queue.empty())
        {
            glm::ivec3 pos = queue.front();
            queue.pop_front();

            uint8_t current_light = get_light_level(pos);
            uint8_t neighbor_light_level = (current_light > 0) ? current_light - 1 : 0;

            if (neighbor_light_level == 0 && current_light <= 1)
                continue;

            for (const auto &offset : neighbors)
            {
                glm::ivec3 neighbor_pos = pos + offset;
                uint8_t potential_light = (offset.y == -1 && current_light == 15) ? 15 : neighbor_light_level;
                uint8_t neighbor_current_light = get_light_level(neighbor_pos);

                if (is_block_transparent(neighbor_pos) && potential_light > neighbor_current_light)
                {
                    set_light_level(neighbor_pos, potential_light);
                    queue.push_back(neighbor_pos);
                }
            }
        }
    }

    std::pair<int, int> World::set_block(glm::ivec3 world_block_pos, BlockType block_type)
    {
        if (world_block_pos.y < 0 || world_block_pos.y >= CHUNK_HEIGHT)
        {
            // Or throw an exception
            return {-1, -1};
        }

        bool old_block_was_transparent = is_block_transparent(world_block_pos);
        Block new_block(block_type);
        bool new_block_is_transparent = new_block.is_transparent();

        auto [chunk_coords, local_coords] = world_to_chunk_coords(static_cast<float>(world_block_pos.x), static_cast<float>(world_block_pos.y), static_cast<float>(world_block_pos.z));
        auto [local_x, local_y, local_z] = local_coords;

        if (old_block_was_transparent == new_block_is_transparent)
        {
            get_or_create_chunk(chunk_coords.first, chunk_coords.second)->set_block(local_x, local_y, local_z, block_type);
            return chunk_coords;
        }

        uint8_t light_level_removed = get_light_level(world_block_pos);

        Chunk *chunk = get_or_create_chunk(chunk_coords.first, chunk_coords.second);
        auto existing_block = chunk->get_block(local_x, local_y, local_z);
        if (existing_block && existing_block->block_type == BlockType::Bedrock)
        {
            return chunk_coords; // Cannot replace bedrock
        }

        chunk->set_block(local_x, local_y, local_z, block_type);

        if (!old_block_was_transparent && new_block_is_transparent)
        {
            propagate_light_addition(world_block_pos);
        }
        else
        {
            propagate_light_removal(world_block_pos, light_level_removed);
        }

        return chunk_coords;
    }
} // namespace flint
