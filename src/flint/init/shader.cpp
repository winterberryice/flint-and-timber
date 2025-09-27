#include "shader.h"

#include <iostream>
#include <stdexcept>

#include "utils.h"

namespace flint::init
{

    WGPUShaderModule create_shader_module(WGPUDevice device, const char *label, const char *wgsl_source)
    {
        WGPUShaderModuleWGSLDescriptor shaderWGSLDesc = {};
        shaderWGSLDesc.chain.next = nullptr;
        shaderWGSLDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        shaderWGSLDesc.code = makeStringView(wgsl_source);

        WGPUShaderModuleDescriptor shaderDesc = {};
        shaderDesc.nextInChain = &shaderWGSLDesc.chain;
        shaderDesc.label = makeStringView(label);

        WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
        if (!shaderModule)
        {
            std::cerr << "Failed to create shader module: " << label << std::endl;
            throw std::runtime_error("Failed to create shader module");
        }

        return shaderModule;
    }

} // namespace flint::init