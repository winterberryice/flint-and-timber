#include "app.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "glm/gtc/matrix_transform.hpp"
#include "flint_utils.h"
#include "shaders/main_shader.wgsl.h"

namespace
{
    void PrintDeviceError(WGPUErrorType type, const char *message, void *)
    {
        std::cerr << "Dawn device error: " << type << " - " << message << std::endl;
    }

    struct AdapterRequestData
    {
        WGPUAdapter adapter = nullptr;
        bool done = false;
    };

    static void OnAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
    {
        AdapterRequestData *data = static_cast<AdapterRequestData *>(userdata1);
        if (status == WGPURequestAdapterStatus_Success)
        {
            data->adapter = adapter;
        }
        else
        {
            std::cerr << "Failed to get adapter" << std::endl;
        }
        data->done = true;
    }

    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    static void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
    {
        DeviceRequestData *data = static_cast<DeviceRequestData *>(userdata1);
        if (status == WGPURequestDeviceStatus_Success)
        {
            data->device = device;
        }
        else
        {
            std::cerr << "Failed to get device" << std::endl;
        }
        data->done = true;
    }
}

namespace flint
{

    // --- ChunkRenderBuffers Destructor and Move Semantics ---
    ChunkRenderBuffers::~ChunkRenderBuffers()
    {
        if (vertex_buffer)
            wgpuBufferRelease(vertex_buffer);
        if (index_buffer)
            wgpuBufferRelease(index_buffer);
    }

    ChunkRenderBuffers::ChunkRenderBuffers(ChunkRenderBuffers &&other) noexcept
        : vertex_buffer(other.vertex_buffer), index_buffer(other.index_buffer), num_indices(other.num_indices)
    {
        other.vertex_buffer = nullptr;
        other.index_buffer = nullptr;
        other.num_indices = 0;
    }

    ChunkRenderBuffers &ChunkRenderBuffers::operator=(ChunkRenderBuffers &&other) noexcept
    {
        if (this != &other)
        {
            if (vertex_buffer)
                wgpuBufferRelease(vertex_buffer);
            if (index_buffer)
                wgpuBufferRelease(index_buffer);
            vertex_buffer = other.vertex_buffer;
            index_buffer = other.index_buffer;
            num_indices = other.num_indices;
            other.vertex_buffer = nullptr;
            other.index_buffer = nullptr;
            other.num_indices = 0;
        }
        return *this;
    }

    App::App(SDL_Window *_window) : window(_window)
    {
        width = 1280;
        height = 720;
        SDL_GetWindowSize(window, (int *)&width, (int *)&height);

        instance = wgpuCreateInstance(nullptr);
        if (!instance) throw std::runtime_error("Failed to create WebGPU instance");

        WGPURequestAdapterOptions adapterOptions = {};
        AdapterRequestData adapterData;
        WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
        adapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        adapterCallbackInfo.callback = OnAdapterReceived;
        adapterCallbackInfo.userdata1 = &adapterData;
        wgpuInstanceRequestAdapter(instance, &adapterOptions, adapterCallbackInfo);
        while (!adapterData.done) { wgpuInstanceProcessEvents(instance); }
        adapter = adapterData.adapter;
        if (!adapter) throw std::runtime_error("No suitable WebGPU adapter found");

        WGPUDeviceDescriptor deviceDesc = {};
        DeviceRequestData deviceData;
        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;
        wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);
        while (!deviceData.done) { wgpuInstanceProcessEvents(instance); }
        device = deviceData.device;
        if (!device) throw std::runtime_error("Failed to create WebGPU device");

        queue = wgpuDeviceGetQueue(device);
        surface = SDL_GetWGPUSurface(instance, window);
        if (!surface) throw std::runtime_error("Failed to create WebGPU surface");

        WGPUSurfaceCapabilities surfaceCapabilities;
        wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCapabilities);
        WGPUTextureFormat surface_format = surfaceCapabilities.formats[0];

        config = {};
        config.device = device;
        config.format = surface_format;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.width = width;
        config.height = height;
        config.presentMode = WGPUPresentMode_Fifo;
        config.alphaMode = surfaceCapabilities.alphaModes[0];
        wgpuSurfaceConfigure(surface, &config);
        wgpuSurfaceCapabilitiesFreeMembers(surfaceCapabilities);
    }

    App::~App()
    {
        // Note: All other resources are currently placeholders or empty and don't require release.
        // As logic is restored, ensure all created WGPU objects are released here.
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuSurfaceRelease(surface);
        wgpuInstanceRelease(instance);
    }

    void App::resize(uint32_t new_width, uint32_t new_height)
    {
        if (new_width > 0 && new_height > 0)
        {
            width = new_width;
            height = new_height;
            config.width = new_width;
            config.height = new_height;
            wgpuSurfaceConfigure(surface, &config);
        }
    }

    void App::handle_input(const SDL_Event &event)
    {
        // Empty for now
    }

    void App::update()
    {
        // Empty for now
    }

    void App::render()
    {
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);

        if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
            if (surface_texture.texture) wgpuTextureRelease(surface_texture.texture);
            wgpuSurfaceConfigure(surface, &config);
            return;
        }

        if (!surface_texture.texture) {
            return;
        }

        WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);
        if (!view) {
            wgpuTextureRelease(surface_texture.texture);
            throw std::runtime_error("Failed to create surface texture view.");
        }

        WGPUCommandEncoderDescriptor enc_desc = {};
        enc_desc.label = flint_utils::makeStringView("Render Encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &enc_desc);

        {
            WGPURenderPassColorAttachment color_attachment = {};
            color_attachment.view = view;
            color_attachment.loadOp = WGPULoadOp_Clear;
            color_attachment.storeOp = WGPUStoreOp_Store;
            color_attachment.clearValue = {0.1, 0.2, 0.3, 1.0}; // Blue sky color

            WGPURenderPassDescriptor pass_desc = {};
            pass_desc.label = flint_utils::makeStringView("Clear Screen Pass");
            pass_desc.colorAttachmentCount = 1;
            pass_desc.colorAttachments = &color_attachment;
            pass_desc.depthStencilAttachment = nullptr;

            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);
            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);
        }

        WGPUCommandBufferDescriptor cmd_buff_desc = {};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buff_desc);

        wgpuQueueSubmit(queue, 1, &cmd_buffer);
        wgpuSurfacePresent(surface);

        wgpuTextureViewRelease(view);
        wgpuTextureRelease(surface_texture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }

    void App::process_mouse_motion(float delta_x, float delta_y)
    {
    }

    void App::set_mouse_grab(bool grab)
    {
    }

    void App::build_or_rebuild_chunk_mesh(int chunk_cx, int chunk_cz) {
    }

} // namespace flint
