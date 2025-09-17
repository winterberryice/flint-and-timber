#pragma once

namespace flint::graphics {

const char* SHADER_WGSL = R"(
struct CameraUniform {
    view_proj: mat4x4<f32>,
}
@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
    @location(2) sky_light: u32,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) @interpolate(flat) sky_light: u32,
};

@vertex
fn vs_main(model: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_proj * vec4<f32>(model.position, 1.0);
    out.color = model.color;
    out.sky_light = model.sky_light;
    return out;
}

fn cubic_bezier(p0: f32, p1: f32, p2: f32, p3: f32, t: f32) -> f32 {
    let t_inv = 1.0 - t;
    let t_inv_sq = t_inv * t_inv;
    let t_sq = t * t;

    return (t_inv_sq * t_inv * p0) +
           (3.0 * t_inv_sq * t * p1) +
           (3.0 * t_inv * t_sq * p2) +
           (t_sq * t * p3);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let normalized_light = f32(in.sky_light) / 15.0;

    // Non-linear brightness curve
    let bezier_y = cubic_bezier(0.0, 0.0, 0.1, 1.0, normalized_light);
    let light_intensity = 0.05 + bezier_y * 0.95;

    let final_color = in.color * light_intensity;

    return vec4<f32>(final_color, 1.0);
}
)";

} // namespace flint::graphics
