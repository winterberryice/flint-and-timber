# Translation Summary: Rust Engine to C++

This document provides a summary of the automated translation from the Rust engine in `__rust__/engine` to the C++ project in `src_from_rust`.

## 1. High-Fidelity Translations

These files were translated with high fidelity, meaning their structure, logic, and data representations are nearly 1:1 with the original Rust code. They form the core, non-rendering data structures of the application.

- **`block.h`, `block.cpp`**: Represents a single block in the world. `BlockType` enum and `Block` struct are directly translated.
- **`physics.hpp`**: Contains physics constants and the `AABB` (Axis-Aligned Bounding Box) struct. Translated to a single header file for convenience.
- **`raycast.h`, `raycast.cpp`**: Voxel raycasting logic. The algorithm is preserved, with `glam` math replaced by `glm`.
- **`camera.h`, `camera.cpp`**: `CameraUniform` struct for passing view-projection matrix to shaders.
- **`player.h`, `player.cpp`**: The `Player` class and its physics/collision logic. The complex `update_physics_and_collision` method was fully translated.
- **`chunk.h`, `chunk.cpp`**: The `Chunk` class, including terrain/tree generation and the initial sky lighting algorithm. `rand` was replaced with C++'s `<random>`.
- **`world.h`, `world.cpp`**: The `World` class, managing a map of chunks. The complex dynamic light propagation logic was fully translated.
- **`ui/item.h`, `ui/item.cpp`**: `ItemType` and `ItemStack` structs for the inventory system.
- **`cube_geometry.h`, `cube_geometry.cpp`**: Defines the vertex data for a cube's faces. The `lazy_static` initialization was replicated with a C++ static variable.

## 2. Files Requiring Manual Work & TODOs

These files contain stubbed implementations or significant `// TODO:` comments because they relied on external Rust libraries or would require complex, manual implementation of core loops.

- **`app.h`, `app.cpp`**: This is the main application class, equivalent to `State` in Rust.
    - **Missing Logic:** The constructor and main methods (`update`, `render`, `handle_input`) are largely placeholders. The core application loop, which ties all the modules together for rendering and updates, needs to be implemented.
    - **Dependencies:** Requires a build system (like CMake) to link against `SDL3` and `dawn`.
- **`ui/ui_text.h`, `ui/ui_text.cpp`**: This is a stub module.
    - **Reason:** The original code used the `wgpu_text` library for all text rendering. As per instructions, no new third-party dependencies were added.
    - **Required Work:** A text rendering solution needs to be chosen and integrated (e.g., using `SDL_ttf` or `FreeType`).
- **`debug_overlay.h`, `debug_overlay.cpp`**: Similar to `ui_text`, the rendering part is a stub.
    - **Reason:** Depends on `wgpu_text`.
    - **Existing Logic:** The logic for calculating FPS and formatting the debug string is fully translated and functional. Only the final rendering call is missing.
- **`texture.h`, `texture.cpp`**:
    - **Reason:** The original code used the `image` crate to load texture data from memory.
    - **Required Work:** The `load_from_rgba` function was created as a replacement, but code to load image files (e.g., PNGs) from disk into raw RGBA data is needed. A library like `stb_image.h` is a common choice for this.

## 3. Graphics API Mapping (`wgpu` -> `dawn`)

This section highlights files where the translation from `wgpu` (Rust) to `dawn` (C++, via `webgpu/webgpu.h`) was performed. This was one of the most intensive parts of the translation.

- **`ui/item_renderer.h`, `ui/item_renderer.cpp`**: Renders items in the UI (hotbar, inventory, dragged item). Contains a full render pipeline setup.
- **`ui/crosshair.h`, `ui/crosshair.cpp`**: Renders the crosshair.
- **`ui/hotbar.h`, `ui/hotbar.cpp`**: Renders the hotbar background and slots.
- **`ui/inventory.h`, `ui/inventory.cpp`**: Renders the inventory background and slots.
- **`wireframe_renderer.h`, `wireframe_renderer.cpp`**: Renders the selection box around the targeted block.
- **`texture.h`, `texture.cpp`**: Handles creation of GPU texture and sampler resources from raw pixel data.
- **`app.cpp`**: Contains the initial setup for the main `dawn` device, surface, and swap chain, as well as the creation of the main render pipelines and depth texture.

## 4. Windowing & Input Mapping (`winit` -> `SDL3`)

- **`main.cpp`**: Contains the main application entry point. It initializes `SDL3`, creates a window, and runs the main event loop, replacing the `winit` `ApplicationHandler` logic.
- **`input.h`, `input.cpp`**: The `InputState` struct was translated to use SDL event types (`SDL_MouseButtonEvent`, etc.) instead of `winit` events.
- **`app.cpp`**: The `set_mouse_grab` method was translated to use `SDL_SetRelativeMouseMode`. The main event handling in `handle_input` is stubbed but designed to receive `SDL_Event`s.
