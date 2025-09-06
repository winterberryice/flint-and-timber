#include "flint/app.hpp"
#include <iostream>

namespace flint
{

    App::App()
    {
        // Constructor
    }

    App::~App()
    {
        // Destructor - cleanup will be in Terminate()
    }

    bool App::Initialize(int width, int height)
    {
        std::cout << "Initializing app..." << std::endl;

        m_windowWidth = width;
        m_windowHeight = height;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create window
        m_window = SDL_CreateWindow("WebGPU App", m_windowWidth, m_windowHeight, 0);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }

        std::cout << "SDL initialized successfully" << std::endl;

        // TODO: Initialize WebGPU
        // TODO: Create surface
        // TODO: Configure surface

        m_running = true;
        return true;
    }

    void App::Run()
    {
        std::cout << "Running app..." << std::endl;

        while (m_running)
        {
            // TODO: Handle SDL events
            // TODO: Update logic
            // TODO: Render frame

            // For now, just exit immediately to avoid infinite loop
            m_running = false;
        }
    }

    void App::Terminate()
    {
        std::cout << "Terminating app..." << std::endl;

        // TODO: Cleanup WebGPU resources
        // TODO: Cleanup SDL resources

        m_running = false;
    }

} // namespace flint
