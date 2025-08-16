#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_properties.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <iostream>

namespace flint_and_timber {

#define FLINT_AND_TIMBER_WINDOW_WIDTH 1280
#define FLINT_AND_TIMBER_WINDOW_HEIGHT 720

void run() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "Flint & Timber",
        FLINT_AND_TIMBER_WINDOW_WIDTH,
        FLINT_AND_TIMBER_WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return;
    }

    bgfx::PlatformData pd;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        pd.ndt = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        pd.nwh = (void*)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    } else {
        pd.ndt = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    }
#elif BX_PLATFORM_OSX
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
#elif BX_PLATFORM_WINDOWS
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count; // auto-select backend
    bgfx_init.resolution.width = FLINT_AND_TIMBER_WINDOW_WIDTH;
    bgfx_init.resolution.height = FLINT_AND_TIMBER_WINDOW_HEIGHT;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;

    if (!bgfx::init(bgfx_init)) {
        std::cerr << "Failed to initialize bgfx" << std::endl;
        return;
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                bgfx::reset(event.window.data1, event.window.data2, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
            }
        }

        bgfx::touch(0);
        bgfx::frame();
    }

    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace flint_and_timber

int main(int argc, char* argv[]) {
    flint_and_timber::run();
    return 0;
}
