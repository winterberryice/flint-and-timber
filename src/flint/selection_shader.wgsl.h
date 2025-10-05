#pragma once

namespace flint
{
    constexpr const char* WGSL_selection_vertexShaderSource = R"(
struct Uniforms {
    viewProjectionMatrix: mat4x4<f32>,
};
@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
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
    out.clip_position = uniforms.viewProjectionMatrix * vec4<f32>(model.position, 1.0);
    return out;
}
)";

    constexpr const char* WGSL_selection_fragmentShaderSource = R"(
// These bindings are unused but must be declared to match the bind group layout
// of the main rendering pipeline.
@group(0) @binding(1) var t_atlas: texture_2d<f32>;
@group(0) @binding(2) var s_atlas: sampler;

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 1.0, 1.0, 1.0); // White
}
)";
} // namespace flint