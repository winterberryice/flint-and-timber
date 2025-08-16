#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_properties.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <bgfx/embedded_shader.h>

#include "v_simple.sc.h"
#include "f_simple.sc.h"

#include <iostream>


namespace flint_and_timber {

#define FLINT_AND_TIMBER_WINDOW_WIDTH 1280
#define FLINT_AND_TIMBER_WINDOW_HEIGHT 720

struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;

    static void init() {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, true)
            .end();
    };

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_triangleVertices[] = {
    {-0.5f, -0.5f, 0.0f, 0xff0000ff}, // Bottom-left, Red
    { 0.5f, -0.5f, 0.0f, 0xff00ff00}, // Bottom-right, Green
    { 0.0f,  0.5f, 0.0f, 0xffff0000}, // Top-center, Blue
};

static const uint16_t s_triangleIndices[] = {
    0, 1, 2,
};

static const bgfx::EmbeddedShader s_embeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(v_simple),
    BGFX_EMBEDDED_SHADER(f_simple),
    BGFX_EMBEDDED_SHADER_END()
};

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
    bgfx_init.rendererType = bgfx::RendererType::OpenGL;


    if (!bgfx::init(bgfx_init)) {
        std::cerr << "Failed to initialize bgfx" << std::endl;
        return;
    }

    PosColorVertex::init();

    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(s_triangleVertices, sizeof(s_triangleVertices)),
        PosColorVertex::ms_layout
    );

    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(s_triangleIndices, sizeof(s_triangleIndices))
    );

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "v_simple"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "f_simple"),
        true
    );

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
            }
        }

        const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
        const bx::Vec3 eye = {0.0f, 0.0f, -2.0f};

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj, 60.0f, float(FLINT_AND_TIMBER_WINDOW_WIDTH) / float(FLINT_AND_TIMBER_WINDOW_HEIGHT), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewRect(0, 0, 0, FLINT_AND_TIMBER_WINDOW_WIDTH, FLINT_AND_TIMBER_WINDOW_HEIGHT);
        bgfx::setViewTransform(0, view, proj);

        bgfx::touch(0);

        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, program);

        bgfx::frame();
    }

    bgfx::destroy(ibh);
    bgfx::destroy(vbh);
    bgfx::destroy(program);

    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace flint_and_timber

int main(int argc, char* argv[]) {
    flint_and_timber::run();
    return 0;
}
