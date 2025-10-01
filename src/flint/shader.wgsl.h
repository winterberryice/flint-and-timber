#pragma once

namespace flint
{

    inline constexpr const char *WGSL_vertexShaderSource = R"(
struct Uniforms {
    viewProjectionMatrix: mat4x4<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
    @location(2) tex_coords: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) tex_coords: vec2<f32>,
};

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.viewProjectionMatrix * vec4<f32>(input.position, 1.0);
    output.color = input.color;
    output.tex_coords = input.tex_coords;
    return output;
}
)";

    inline constexpr const char *WGSL_fragmentShaderSource = R"(
@group(1) @binding(0) var t_atlas: texture_2d<f32>;
@group(1) @binding(1) var s_atlas: sampler;

// Define the sentinel color for grass top tinting
let GRASS_TOP_TINT_COLOR = vec3<f32>(0.1, 0.9, 0.1);

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let texture_color = textureSample(t_atlas, s_atlas, in.tex_coords);

    // Check if the vertex color is our special sentinel value
    if (in.color == GRASS_TOP_TINT_COLOR) {
        // The texture at (0,0) is grayscale. We tint it by mixing with a green color.
        // The grayscale value is in the red channel of the texture.
        let grayscale = texture_color.r;
        let tinted_color = vec3<f32>(grayscale * 0.4, grayscale * 1.0, grayscale * 0.4);
        return vec4<f32>(tinted_color, 1.0);
    } else {
        // For all other blocks, use the texture color directly.
        return texture_color;
    }
}
)";

} // namespace flint
