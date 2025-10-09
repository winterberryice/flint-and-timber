#pragma once

#include <string_view>

namespace flint
{
    constexpr std::string_view CROSSHAIR_WGSL_vertexShaderSource = R"(@vertex
fn vs_main(@location(0) in_pos: vec2<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(in_pos, 0.0, 1.0);
})";

    constexpr std::string_view CROSSHAIR_WGSL_fragmentShaderSource = R"(@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 1.0, 1.0, 1.0); // White
})";

} // namespace flint