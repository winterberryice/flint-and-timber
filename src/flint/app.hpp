#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.hpp>
#include <optional>
#include <memory>
#include "player.hpp"
#include "world.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "ui/crosshair.hpp"
#include "ui/inventory.hpp"
#include "ui/hotbar.hpp"
#include "ui/item_renderer.hpp"
#include "ui/ui_text.hpp"
#include "wireframe_renderer.hpp"
#include "raycast.hpp"
#include <map>
#include <vector>
#include <utility>

struct ChunkRenderBuffers;
struct ChunkRenderData;

class State {
    // This would contain all the game state and rendering resources.
    // For this rewrite, we are not implementing the details of the rendering engine.
};

class App {
public:
    App();
    ~App();
    void run();

private:
    SDL_Window* window;
    std::unique_ptr<State> state;
    bool running = true;
    bool mouse_grabbed = false;

    void handle_event(const SDL_Event& event);
    void set_mouse_grab(bool grab);
};
