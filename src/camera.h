#pragma once

#include "glm/glm.hpp"

namespace flint {

    // In C++, as long as we use standard layout types like glm::mat4,
    // the memory layout is predictable. This serves a similar purpose to
    // Rust's `#[repr(C)]` and `bytemuck` traits for GPU buffer compatibility.
    struct CameraUniform {
        glm::mat4 view_proj;

        CameraUniform();
    };

} // namespace flint
