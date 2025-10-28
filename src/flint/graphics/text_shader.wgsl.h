#pragma once

#include <string_view>

namespace flint::graphics
{
    constexpr std::string_view TEXT_WGSL_shaderSource = R"shader(
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) sky_light: f32,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4<f32>(in.position.xy, 0.0, 1.0);
    out.uv = in.uv;
    return out;
}

@group(0) @binding(0) var t_font: texture_2d<f32>;
@group(0) @binding(1) var s_font: sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let alpha = textureSample(t_font, s_font, in.uv).r;
    return vec4<f32>(1.0, 1.0, 1.0, alpha);
}
)shader";
}