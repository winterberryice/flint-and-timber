#pragma once

#include <string_view>

namespace flint
{

    constexpr std::string_view WGSL_debugShaderSource = R"(
struct CameraUniform {
    view_proj: mat4x4<f32>,
};
@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct ModelUniform {
    model: mat4x4<f32>,
};
@group(1) @binding(0)
var<uniform> object: ModelUniform;

struct ColorUniform {
    color: vec4<f32>,
};
@group(1) @binding(1)
var<uniform> u_color: ColorUniform;


@vertex
fn vs_main(
    @location(0) in_position: vec3<f32>,
) -> @builtin(position) vec4<f32> {
    return camera.view_proj * object.model * vec4<f32>(in_position, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return u_color.color;
}
)";

} // namespace flint