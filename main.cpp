#include <SDL3/SDL.h>
#include <webgpu/webgpu.hpp>
#include "sdl3webgpu.h"
#include <iostream>
#include <vector>

const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("WebGPU Triangle", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    wgpu::Instance instance = wgpu::createInstance(nullptr);
    wgpu::Surface surface = wgpu::Surface(SDL_GetWGPUSurface(instance.get(), window));

    wgpu::RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = surface;
    wgpu::Adapter adapter = instance.requestAdapter(adapterOpts);

    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.label = "My Device";
    wgpu::Device device = adapter.requestDevice(deviceDesc);

    auto onDeviceError = [](WGPUErrorType type, char const* message, void*) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    device.setUncapturedErrorCallback(onDeviceError, nullptr);

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.code = shaderSource;
    wgpu::ShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &shaderCodeDesc;
    wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);

    wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);

    wgpu::SwapChainDescriptor swapChainDesc{};
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.format = swapChainFormat;
    swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
    wgpu::SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc{};

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = swapChainFormat;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.fragment = &fragmentState;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

    wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();
        if (!nextTexture) {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        wgpu::CommandEncoderDescriptor commandEncoderDesc{};
        commandEncoderDesc.label = "Command Encoder";
        wgpu::CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

        wgpu::RenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = {0.0, 0.0, 0.0, 1.0};

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.label = "My Render Pass";

        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.setPipeline(pipeline);
        renderPass.draw(3, 1, 0, 0);
        renderPass.end();

        wgpu::CommandBufferDescriptor cmdBuffDesc{};
        cmdBuffDesc.label = "Command Buffer";
        wgpu::CommandBuffer commands = encoder.finish(cmdBuffDesc);
        device.getQueue().submit(1, &commands);

        swapChain.present();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
