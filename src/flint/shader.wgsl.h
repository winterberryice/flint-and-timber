#pragma once

namespace flint
{

    inline constexpr const char *WGSL_vertexShaderSource = R"(
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
    @location(1) color: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) uv: vec2<f32>,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    // The world renderer will pass an identity matrix for the model.
    let world_position = model_uniform.model * vec4<f32>(in.position, 1.0);
    out.position = camera.view_proj * world_position;
    out.color = in.color;
    out.uv = in.uv;
    return out;
}
)";

    inline constexpr const char *WGSL_fragmentShaderSource = R"(
@group(0) @binding(2) var t_atlas: texture_2d<f32>;
@group(0) @binding(3) var s_atlas: sampler;

struct FragmentInput {
    @location(0) color: vec3<f32>,
    @location(1) uv: vec2<f32>,
};

// A sentinel color to indicate that the texture should be tinted.
// This is used for blocks like leaves, where the base texture is grayscale
// and we want to apply a biome-specific color.
const TINT_SENTINEL = vec3<f32>(0.1, 0.9, 0.1);

@fragment
fn fs_main(in: FragmentInput) -> @location(0) vec4<f32> {
    let texture_color = textureSample(t_atlas, s_atlas, in.uv);

    // Alpha test for transparent textures (e.g., leaves).
    // If the alpha value is below a threshold, discard the fragment.
    if (texture_color.a < 0.1) {
        discard;
    }

    // If the vertex color is the sentinel value, tint the texture.
    // Otherwise, use the texture color directly.
    if (all(in.color == TINT_SENTINEL)) {
        // We assume the texture is grayscale, so we can just use one channel (e.g., R)
        // and multiply it by the desired tint color.
        // For now, we'll just hardcode a green tint for demonstration.
        let tint_color = vec3<f32>(0.2, 0.8, 0.2); // A nice green
        return vec4<f32>(texture_color.r * tint_color, texture_color.a);
    } else {
        // Regular texturing, multiply by vertex color for lighting/shading
        return vec4<f32>(texture_color.rgb * in.color, texture_color.a);
    }
}
)";

} // namespace flint