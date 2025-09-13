#include "app.hpp"
#include <iostream>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    // Helper for synchronous adapter request
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
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get adapter: " << errorMsg << " (status: " << status << ")" << std::endl;
        }
        data->done = true;
    }

    // Helper for synchronous device request
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
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get device: " << errorMsg << " (status: " << status << ")" << std::endl;
        }
        data->done = true;
    }
}

namespace flint
{
    App::App()
        : m_player(glm::vec3(8.0f, 80.0f, 8.0f)),
          m_camera(glm::vec3(8.0f, 80.0f, 8.0f), 0.0f, 0.0f)
    {
    }

    bool App::Initialize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        m_window = SDL_CreateWindow("Flint", m_windowWidth, m_windowHeight, 0);
        if (!m_window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }

        m_instance = wgpuCreateInstance(nullptr);
        if (!m_instance)
        {
            std::cerr << "Failed to create WebGPU instance" << std::endl;
            return false;
        }

        // Request adapter
        WGPURequestAdapterOptions adapterOptions = {};
        AdapterRequestData adapterData;
        WGPURequestAdapterCallbackInfo callbackInfo = {};
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.callback = OnAdapterReceived;
        callbackInfo.userdata1 = &adapterData;
        wgpuInstanceRequestAdapter(m_instance, &adapterOptions, callbackInfo);
        while (!adapterData.done)
        {
            wgpuInstanceProcessEvents(m_instance);
        }
        m_adapter = adapterData.adapter;
        if (!m_adapter)
        {
            std::cerr << "No suitable WebGPU adapter found" << std::endl;
            return false;
        }

        // Request device
        WGPUDeviceDescriptor deviceDesc = {};
        DeviceRequestData deviceData;
        WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
        deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        deviceCallbackInfo.callback = OnDeviceReceived;
        deviceCallbackInfo.userdata1 = &deviceData;
        wgpuAdapterRequestDevice(m_adapter, &deviceDesc, deviceCallbackInfo);
        while (!deviceData.done)
        {
            wgpuInstanceProcessEvents(m_instance);
        }
        m_device = deviceData.device;
        if (!m_device)
        {
            std::cerr << "Failed to create WebGPU device" << std::endl;
            return false;
        }

        m_queue = wgpuDeviceGetQueue(m_device);
        m_surface = SDL_GetWGPUSurface(m_instance, m_window);

        WGPUSurfaceCapabilities surfaceCapabilities;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &surfaceCapabilities);
        m_surfaceFormat = surfaceCapabilities.formats[0];

        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.device = m_device;
        surfaceConfig.format = m_surfaceFormat;
        surfaceConfig.width = m_windowWidth;
        surfaceConfig.height = m_windowHeight;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        wgpuSurfaceConfigure(m_surface, &surfaceConfig);

        // Initialize game components
        m_world.initialize(m_device, m_queue);
        m_item_renderer.initialize(m_device, m_surfaceFormat);
        m_crosshair.initialize(m_device, m_surfaceFormat);
        m_hotbar.initialize(m_device, m_surfaceFormat);
        m_wireframe_renderer.initialize(m_device, m_surfaceFormat);

        set_mouse_grab(true);
        m_running = true;
        return true;
    }

    void App::Run()
    {
        Uint64 last_time = SDL_GetPerformanceCounter();

        while (m_running)
        {
            Uint64 current_time = SDL_GetPerformanceCounter();
            float delta_time = (current_time - last_time) / (float)SDL_GetPerformanceFrequency();
            last_time = current_time;

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                handle_event(event);
            }

            m_player.update(delta_time, m_input, m_world);
            m_camera.follow_player(m_player);

            render();

            m_input.end_frame();
        }
    }

    void App::handle_event(const SDL_Event &event)
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            m_running = false;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            m_windowWidth = event.window.data1;
            m_windowHeight = event.window.data2;
            // Reconfigure surface
            WGPUSurfaceConfiguration surfaceConfig = {};
            surfaceConfig.device = m_device;
            surfaceConfig.format = m_surfaceFormat;
            surfaceConfig.width = m_windowWidth;
            surfaceConfig.height = m_windowHeight;
            surfaceConfig.presentMode = WGPUPresentMode_Fifo;
            surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
            wgpuSurfaceConfigure(m_surface, &surfaceConfig);
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                set_mouse_grab(!m_mouse_grabbed);
            }
        }

        if (m_mouse_grabbed)
        {
            m_input.handle_event(event);
        }
    }

    void App::set_mouse_grab(bool grab)
    {
        m_mouse_grabbed = grab;
        SDL_SetRelativeMouseMode(m_mouse_grabbed ? SDL_TRUE : SDL_FALSE);
    }

    void App::render()
    {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            return;
        }

        WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

        WGPUCommandEncoderDescriptor encoderDesc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

        {
            WGPURenderPassColorAttachment colorAttachment = {};
            colorAttachment.view = view;
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            colorAttachment.clearValue = {0.1f, 0.1f, 0.2f, 1.0f};

            WGPURenderPassDescriptor renderPassDesc = {};
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;

            WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

            // Render world
            m_world.render(pass, m_camera);

            // Render UI
            m_crosshair.render(pass, m_windowWidth, m_windowHeight);
            m_hotbar.render(pass, m_item_renderer);

            wgpuRenderPassEncoderEnd(pass);
        }

        WGPUCommandBufferDescriptor cmdBufDesc = {};
        WGPUCommandBuffer cmdBuf = wgpuCommandEncoderFinish(encoder, &cmdBufDesc);
        wgpuQueueSubmit(m_queue, 1, &cmdBuf);

        wgpuSurfacePresent(m_surface);

        wgpuTextureViewRelease(view);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmdBuf);
    }

    void App::Terminate()
    {
        wgpuSurfaceRelease(m_surface);
        wgpuQueueRelease(m_queue);
        wgpuDeviceRelease(m_device);
        wgpuAdapterRelease(m_adapter);
        wgpuInstanceRelease(m_instance);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
} // namespace flint
