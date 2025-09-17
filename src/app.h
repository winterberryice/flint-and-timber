#pragma once

#include "block.h"
#include "camera.h"
#include "chunk.h"
#include "cube_geometry.h"
#include "debug_overlay.h"
#include "input.h"
#include "physics.hpp"
#include "player.h"
#include "raycast.h"
#include "texture.h"
#include "ui/crosshair.h"
#include "ui/hotbar.h"
#include "ui/inventory.h"
#include "ui/item.h"
#include "ui/item_renderer.h"
#include "ui/ui_text.h"
#include "wireframe_renderer.h"
#include "world.h"

#include <webgpu/webgpu.h>
#include <SDL3/SDL.h>
#include <sdl3webgpu.h>
#include <glm/glm.hpp>

#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

#include "vertex.h"

namespace flint
{

    struct ChunkRenderBuffers
    {
        WGPUBuffer vertex_buffer = nullptr;
        WGPUBuffer index_buffer = nullptr;
        uint32_t num_indices = 0;

        ~ChunkRenderBuffers();
        ChunkRenderBuffers(const ChunkRenderBuffers &) = delete;
        ChunkRenderBuffers &operator=(const ChunkRenderBuffers &) = delete;
        ChunkRenderBuffers(ChunkRenderBuffers &&other) noexcept;
        ChunkRenderBuffers &operator=(ChunkRenderBuffers &&other) noexcept;
        ChunkRenderBuffers() = default;
    };

    struct ChunkRenderData
    {
        std::optional<ChunkRenderBuffers> opaque_buffers;
        std::optional<ChunkRenderBuffers> transparent_buffers;
    };

    struct AppState
    {
        Player player;

        // TODO when ui is implemented
        // // UI and Overlays
        // DebugOverlay debug_overlay;
        // ui::Crosshair crosshair;
        // ui::Hotbar hotbar;
        // WireframeRenderer wireframe_renderer;
        // ui::ItemRenderer item_renderer;
        // ui::UIText ui_text;
    };

    class App
    {
    public:
        App(SDL_Window *window);
        ~App();

        void resize(uint32_t width, uint32_t height);
        void handle_input(const SDL_Event &event);
        void update();
        void render();
        void process_mouse_motion(float delta_x, float delta_y);
        void set_mouse_grab(bool grab);

    private:
        void build_or_rebuild_chunk_mesh(int chunk_cx, int chunk_cz);
        void handle_inventory_interaction();
        void handle_block_interactions();

        SDL_Window *window;
        uint32_t width;
        uint32_t height;

        // Dawn objects
        WGPUInstance instance = nullptr;
        WGPUSurface surface = nullptr;
        WGPUAdapter adapter = nullptr;
        WGPUDevice device = nullptr;
        WGPUQueue queue = nullptr;
        WGPUSurfaceConfiguration config = {};

        WGPURenderPipeline render_pipeline = nullptr;
        WGPURenderPipeline transparent_render_pipeline = nullptr;

        WGPUTexture depth_texture = nullptr;
        WGPUTextureView depth_texture_view = nullptr;

        // App state
        std::optional<AppState> state;

        World world;
        std::unordered_map<std::pair<int, int>, ChunkRenderData, PairHash> chunk_render_data;
        std::vector<std::pair<int, int>> active_chunk_coords;

        // Camera
        CameraUniform camera_uniform;
        WGPUBuffer camera_buffer = nullptr;
        WGPUBindGroup camera_bind_group = nullptr;
        WGPUBindGroupLayout camera_bind_group_layout = nullptr;

        // Textures
        std::optional<Texture> block_atlas_texture;
        WGPUBindGroup block_atlas_bind_group = nullptr;
        WGPUBindGroupLayout texture_bind_group_layout = nullptr;

        bool inventory_open = false;
        std::optional<ui::ItemStack> dragged_item;

        // Input
        InputState input_state;
        std::optional<std::pair<int, int>> last_mouse_position;
        bool mouse_grabbed = false;

        // Block selection
        std::optional<std::pair<glm::ivec3, BlockFace>> selected_block;
    };

} // namespace flint
