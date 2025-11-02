#include "debug_screen_renderer.h"

#include <iostream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>

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

    void DebugScreenRenderer::begin_frame()
    {
        // Start ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Create a simple text overlay (like Minecraft HUD)
        // Position in top-left corner with no window decorations
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoMove;

        ImGui::Begin("Overlay", nullptr, window_flags);
        ImGui::Text("Hello World");
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
