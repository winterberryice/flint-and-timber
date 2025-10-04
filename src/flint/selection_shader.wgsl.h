#pragma once

namespace flint {
namespace shaders {

const char* const selection_shader_wgsl = R"(
struct CameraUniform {
    view_proj: mat4x4<f32>,
};
@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct ModelUniform {
    model: mat4x4<f32>,
};
@group(1) @binding(0)
var<uniform> model_uniform: ModelUniform;

struct VertexInput {
    @location(0) position: vec3<f32>,
};

@vertex
fn vs_main(
    model_in: VertexInput,
) -> @builtin(position) vec4<f32> {
    return camera.view_proj * model_uniform.model * vec4<f32>(model_in.position, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    // Semi-transparent white for highlighting the selection
    return vec4<f32>(1.0, 1.0, 1.0, 0.2);
}
)";

} // namespace shaders
} // namespace flint