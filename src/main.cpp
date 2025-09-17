#include "app.h"
#include <SDL3/SDL.h>
#include <iostream>

// Forward declaration for the function in app.cpp that gets the surface
// TODO: This is not a clean way to do this. A better design would be to pass the window to the App constructor.
// I have updated the App constructor to take the window, so this is not needed.

namespace
{
    void printVideoSystemInfo()
    {
        // Get the current video driver
        const char *videoDriver = SDL_GetCurrentVideoDriver();

        printf("\n=== SDL3 Video System Information ===\n");
        printf("Current Video Driver: %s\n", videoDriver ? videoDriver : "NULL");

        if (videoDriver)
        {
            if (strcmp(videoDriver, "wayland") == 0)
            {
                printf("🌊 Running on WAYLAND\n");
            }
            else if (strcmp(videoDriver, "x11") == 0)
            {
                printf("🖥️  Running on X11\n");
            }
            else
            {
                printf("🤔 Running on: %s (other)\n", videoDriver);
            }
        }
        else
        {
            printf("❌ No video driver detected\n");
        }

        // List all available video drivers
        int numDrivers = SDL_GetNumVideoDrivers();
        printf("\nAvailable video drivers (%d total):\n", numDrivers);
        for (int i = 0; i < numDrivers; i++)
        {
            const char *driverName = SDL_GetVideoDriver(i);
            bool isCurrent = (videoDriver && strcmp(videoDriver, driverName) == 0);
            printf("  %s%s%s\n",
                   isCurrent ? "[ACTIVE] " : "         ",
                   driverName ? driverName : "NULL",
                   isCurrent ? " ←" : "");
        }
    }

    void printDetailedVideoInfo(SDL_Window *window)
    {
        const char *videoDriver = SDL_GetCurrentVideoDriver();

        printf("\n=== Detailed Video System Info ===\n");
        printf("SDL Video Driver: %s\n", videoDriver ? videoDriver : "NULL");

        if (!videoDriver)
            return;

        if (strcmp(videoDriver, "wayland") == 0)
        {
            printf("🌊 **WAYLAND SESSION DETECTED** 🌊\n");

            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);

                // Check for Wayland-specific properties
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER))
                {
                    void *waylandDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
                    void *waylandSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);

                    printf("  Wayland Display: %p\n", waylandDisplay);
                    printf("  Wayland Surface: %p\n", waylandSurface);
                    printf("  ✓ Native Wayland window detected\n");
                }
                else
                {
                    printf("  ❌ Wayland properties not available (running via XWayland?)\n");
                }
            }
        }
        else if (strcmp(videoDriver, "x11") == 0)
        {
            printf("🖥️  **X11 SESSION DETECTED** 🖥️\n");

            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);

                // Check for X11-specific properties
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER))
                {
                    void *x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
                    Uint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

                    printf("  X11 Display: %p\n", x11Display);
                    printf("  X11 Window: 0x%llx\n", (unsigned long long)x11Window);
                    printf("  ✓ Native X11 window detected\n");
                }
                else
                {
                    printf("  ❌ X11 properties not available\n");
                }
            }
        }
        else
        {
            printf("🤔 **OTHER DRIVER: %s** 🤔\n", videoDriver);
        }

        // Environment variable checks (additional context)
        const char *waylandDisplay = getenv("WAYLAND_DISPLAY");
        const char *x11Display = getenv("DISPLAY");
        const char *xdgSessionType = getenv("XDG_SESSION_TYPE");

        printf("\n=== Environment Variables ===\n");
        printf("WAYLAND_DISPLAY: %s\n", waylandDisplay ? waylandDisplay : "not set");
        printf("DISPLAY: %s\n", x11Display ? x11Display : "not set");
        printf("XDG_SESSION_TYPE: %s\n", xdgSessionType ? xdgSessionType : "not set");
    }
}

int main(int argc, char *argv[])
{
    std::cout << "Initializing app..." << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    printVideoSystemInfo();

    // Create window
    SDL_Window *window = SDL_CreateWindow("Flint & Timber", 1280, 720, 0);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    printDetailedVideoInfo(window);

    std::cout << "SDL initialized successfully" << std::endl;
    // ===========================

    try
    {
        flint::App app(window);

        bool running = true;
        while (running)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                app.handle_input(event); // Let the app handle event logic
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                }
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        // TODO: This is a simplified exit condition.
                        // The original Rust code has more complex logic for Esc key.
                        running = false;
                    }
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION)
                {
                    app.process_mouse_motion(event.motion.xrel, event.motion.yrel);
                }
            }

            app.update();
            app.render();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
