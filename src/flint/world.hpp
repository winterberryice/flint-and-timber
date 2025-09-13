#pragma once

#include "chunk.hpp"
#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <utility> // for std::pair
#include <deque>
#include <tuple>
#include <webgpu/webgpu.hpp>
#include "camera.hpp"

namespace flint
{
    class World
    {
    public:
        World();
        void initialize(wgpu::Device device, wgpu::Queue queue);
        void render(wgpu::RenderPassEncoder pass, wgpu::Buffer camera_buffer);

        Chunk *get_or_create_chunk(int chunk_x, int chunk_z);
        const Chunk *get_chunk(int chunk_x, int chunk_z) const;
        std::optional<Block> get_block_at_world(float world_x, float world_y, float world_z) const;

        // Returns the chunk coordinates
        std::pair<int, int> set_block(glm::ivec3 world_block_pos, BlockType block_type);

    private:
        std::map<std::pair<int, int>, Chunk> chunks;
        wgpu::RenderPipeline pipeline;
        wgpu::BindGroup bind_group;

        uint8_t get_light_level(glm::ivec3 pos) const;
        void set_light_level(glm::ivec3 pos, uint8_t level);
        bool is_block_transparent(glm::ivec3 pos) const;

        void propagate_light_addition(glm::ivec3 new_air_block_pos);
        void propagate_light_removal(glm::ivec3 new_solid_block_pos, uint8_t light_level_removed);

        void run_light_propagation_queue(std::deque<glm::ivec3> &queue);
        bool should_be_relit(glm::ivec3 pos) const;

        static std::pair<std::pair<int, int>, std::tuple<int, int, int>> world_to_chunk_coords(float world_x, float world_y, float world_z);
    };
} // namespace flint
