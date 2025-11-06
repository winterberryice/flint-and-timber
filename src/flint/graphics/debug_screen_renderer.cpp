#include "debug_screen_renderer.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <glm/glm.hpp>

#include "../player.h"
#include "../world.h"
#include "../block.h"

namespace flint::graphics
{

    DebugScreenRenderer::DebugScreenRenderer() = default;

    DebugScreenRenderer::~DebugScreenRenderer() = default;

    void DebugScreenRenderer::init(SDL_Window *window, WGPUDevice device, WGPUTextureFormat surfaceFormat)
    {
        std::cout << "Initializing debug screen renderer..." << std::endl;

        m_window = window;
        m_device = device;

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

        // Setup ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForOther(m_window);
        ImGui_ImplWGPU_InitInfo init_info = {};
        init_info.Device = m_device;
        init_info.NumFramesInFlight = 3;
        init_info.RenderTargetFormat = (WGPUTextureFormat)surfaceFormat;
        init_info.DepthStencilFormat = (WGPUTextureFormat)WGPUTextureFormat_Undefined;
        ImGui_ImplWGPU_Init(&init_info);

        std::cout << "Debug screen renderer initialized." << std::endl;
    }

    void DebugScreenRenderer::process_event(const SDL_Event &event)
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void DebugScreenRenderer::begin_frame(const flint::player::Player &player, const flint::World &world)
    {
        // Start ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Create a simple text overlay (like Minecraft HUD)
        // Position in top-left corner with no window decorations
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f); // Semi-transparent background
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoMove;

        ImGui::Begin("Overlay", nullptr, window_flags);

        // Get player data
        glm::vec3 pos = player.get_position();
        float yaw = player.get_yaw();
        float pitch = player.get_pitch();

        // Display position (Minecraft uses X Y Z format)
        ImGui::Text("XYZ: %.3f / %.3f / %.3f", pos.x, pos.y, pos.z);

        // Display block position (feet position)
        glm::ivec3 block_pos = glm::floor(pos);
        ImGui::Text("Block: %d %d %d", block_pos.x, block_pos.y, block_pos.z);

        // Display facing direction
        ImGui::Text("Facing: yaw %.1f pitch %.1f", yaw, pitch);

        // Display looked-at block
        auto selected_block = player.get_selected_block();
        if (selected_block.has_value())
        {
            glm::ivec3 block = selected_block->block_position;
            ImGui::Text("Looking at: %d %d %d", block.x, block.y, block.z);
        }
        else
        {
            ImGui::Text("Looking at: none");
        }

        // Display light level at feet
        const Block *feet_block = world.getBlock(block_pos.x, block_pos.y, block_pos.z);
        if (feet_block)
        {
            ImGui::Text("Light: %d", feet_block->sky_light);
        }
        else
        {
            ImGui::Text("Light: N/A");
        }

        ImGui::End();

        // Finalize ImGui frame
        ImGui::Render();
    }

    void DebugScreenRenderer::render(WGPURenderPassEncoder renderPass)
    {
        // Render ImGui
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
    }

    void DebugScreenRenderer::cleanup()
    {
        std::cout << "Cleaning up debug screen renderer..." << std::endl;

        // Cleanup ImGui
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

} // namespace flint::graphics
