#pragma once

// A simple shader for drawing the block selection outline.
// It doesn't use textures, only a solid color.
static const char *WGSL_outline_shader_source = R"(
struct Uniforms {
    modelViewProjectionMatrix: mat4x4<f32>,
};
@binding(0) @group(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    // We don't use normal and uv, but the vertex layout is shared, so they are still in the input struct.
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = uniforms.modelViewProjectionMatrix * vec4<f32>(in.position, 1.0);
    return out;
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    // Return a solid white color. We use white so it's visible against any block.
    return vec4<f32>(1.0, 1.0, 1.0, 1.0);
}
)";