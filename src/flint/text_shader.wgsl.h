#pragma once

#include <string>

namespace flint::shaders
{
    const std::string text_shader_source = R"(
        @group(0) @binding(0) var my_texture: texture_2d<f32>;
        @group(0) @binding(1) var my_sampler: sampler;

        struct VertexInput {
            @location(0) position: vec2<f32>,
            @location(1) tex_coords: vec2<f32>,
        };

        struct VertexOutput {
            @builtin(position) clip_position: vec4<f32>,
            @location(0) tex_coords: vec2<f32>,
        };

        @vertex
        fn vs_main(
            model: VertexInput,
        ) -> VertexOutput {
            var out: VertexOutput;
            out.clip_position = vec4<f32>(model.position, 0.0, 1.0);
            out.tex_coords = model.tex_coords;
            return out;
        }

        @fragment
        fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
            let alpha = textureSample(my_texture, my_sampler, in.tex_coords).r;
            return vec4<f32>(1.0, 1.0, 1.0, alpha); // White text
        }
    )";
}
