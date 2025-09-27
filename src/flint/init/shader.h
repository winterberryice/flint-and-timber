#pragma once

#include <webgpu/webgpu.h>

namespace flint::init
{

    WGPUShaderModule create_shader_module(WGPUDevice device, const char *label, const char *wgsl_source);

} // namespace flint::init