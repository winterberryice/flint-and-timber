#pragma once

#include <string_view>

namespace flint
{
    constexpr std::string_view SELECTION_WGSL_vertexShaderSource = R"(
struct CameraUniform {
    view_proj: mat4x4<f32>,
};
@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct ModelUniform {
    model: mat4x4<f32>,
};
@group(0) @binding(1)
var<uniform> model_uniform: ModelUniform;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
};

@vertex
fn vs_main(
    model: VertexInput,
) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_proj * model_uniform.model * vec4<f32>(model.position, 1.0);
    return out;
}
)";

    constexpr std::string_view SELECTION_WGSL_fragmentShaderSource = R"(
@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(0.2, 0.2, 0.2, 1.0); 
}
)";
}