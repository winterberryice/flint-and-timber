#include "wgpu.h"

#include <iostream>
#include <future>
#include <chrono>

namespace
{
    static void OnAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
    {
        std::cout << "OnAdapterReceived called with status: " << status << std::endl;

        if (status == WGPURequestAdapterStatus_Success)
        {
            std::cout << "Adapter request successful" << std::endl;
            static_cast<std::promise<WGPUAdapter> *>(userdata1)->set_value(adapter);
        }
        else
        {
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get adapter: " << errorMsg << " (status: " << status << ")" << std::endl;
            static_cast<std::promise<WGPUAdapter> *>(userdata1)->set_value(nullptr);
        }
    }

    static void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
    {
        std::cout << "OnDeviceReceived called with status: " << status << std::endl;

        if (status == WGPURequestDeviceStatus_Success)
        {
            std::cout << "Device request successful" << std::endl;
            static_cast<std::promise<WGPUDevice> *>(userdata1)->set_value(device);
        }
        else
        {
            std::string errorMsg = "Unknown error";
            if (message.data && message.length > 0)
            {
                errorMsg = std::string(message.data, message.length);
            }
            std::cerr << "Failed to get device: " << errorMsg << " (status: " << status << ")" << std::endl;
            static_cast<std::promise<WGPUDevice> *>(userdata1)->set_value(nullptr);
        }
    }
}

void flint::init::wgpu(const uint32_t width, const uint32_t height, SDL_Window *window, WGPUInstance &out_instance, WGPUSurface &out_surface, WGPUTextureFormat &out_surface_format, WGPUAdapter &out_adapter, WGPUDevice &out_device, WGPUQueue &out_queue)
{
    // Initialize WebGPU instance
    auto m_instance = wgpuCreateInstance(nullptr);
    out_instance = m_instance;
    if (!m_instance)
    {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        throw std::runtime_error("Failed to create WebGPU instance");
    }

    std::cout << "WebGPU instance created" << std::endl;

    std::promise<WGPUAdapter> adapter_promise;
    auto adapter_future = adapter_promise.get_future();

    // Request adapter
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.compatibleSurface = nullptr;
    adapterOptions.powerPreference = WGPUPowerPreference_Undefined;
    adapterOptions.backendType = WGPUBackendType_Undefined;
    adapterOptions.forceFallbackAdapter = false;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    callbackInfo.callback = OnAdapterReceived;
    callbackInfo.userdata1 = &adapter_promise;
    callbackInfo.userdata2 = nullptr;

    std::cout << "Requesting WebGPU adapter..." << std::endl;
    wgpuInstanceRequestAdapter(m_instance, &adapterOptions, callbackInfo);
    while (adapter_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        // Process all pending WebGPU events and trigger callbacks.
        wgpuInstanceProcessEvents(m_instance);
    }
    auto m_adapter = adapter_future.get();
    if (!m_adapter)
    {
        throw std::runtime_error("Failed to get WGPU adapter.");
    }
    out_adapter = m_adapter;
    std::cout << "WebGPU adapter obtained successfully" << std::endl;

    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = {nullptr, 0};
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredFeatures = nullptr;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = {nullptr, 0};

    std::promise<WGPUDevice> device_promise;
    auto device_future = device_promise.get_future();

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.nextInChain = nullptr;
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    deviceCallbackInfo.callback = OnDeviceReceived;
    deviceCallbackInfo.userdata1 = &device_promise;
    deviceCallbackInfo.userdata2 = nullptr;

    std::cout << "Requesting WebGPU device..." << std::endl;
    wgpuAdapterRequestDevice(m_adapter, &deviceDesc, deviceCallbackInfo);

    std::cout << "Processing events until device callback..." << std::endl;
    while (device_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        // Process all pending WebGPU events and trigger callbacks.
        wgpuInstanceProcessEvents(m_instance);
    }
    auto m_device = device_future.get();
    if (!m_device)
    {
        std::cerr << "Failed to create WebGPU device" << std::endl;
        throw std::runtime_error("Failed to create WebGPU device");
    }
    out_device = m_device;
    std::cout << "WebGPU device created successfully" << std::endl;

    // Get the queue
    auto m_queue = wgpuDeviceGetQueue(m_device);
    out_queue = m_queue;
    std::cout << "WebGPU queue obtained" << std::endl;

    // Create surface
    auto m_surface = SDL_GetWGPUSurface(m_instance, window);
    out_surface = m_surface;
    if (!m_surface)
    {
        std::cerr << "Failed to create WebGPU surface" << std::endl;
        throw std::runtime_error("Failed to create WebGPU surface");
    }
    std::cout << "WebGPU surface created successfully" << std::endl;

    // Get supported surface formats
    WGPUSurfaceCapabilities surfaceCapabilities;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &surfaceCapabilities);

    // Use the first supported format (this is the preferred one)
    auto m_surfaceFormat = surfaceCapabilities.formats[0];
    out_surface_format = m_surfaceFormat;
    std::cout << "Using surface format: " << m_surfaceFormat << std::endl;

    // Configure the surface
    WGPUSurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.device = m_device;
    surfaceConfig.format = m_surfaceFormat; // Common format
    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfig.width = width;
    surfaceConfig.height = height;
    surfaceConfig.presentMode = WGPUPresentMode_Fifo;
    surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;
    wgpuSurfaceCapabilitiesFreeMembers(surfaceCapabilities);
    //

    wgpuSurfaceConfigure(m_surface, &surfaceConfig);
    std::cout << "WebGPU surface configured" << std::endl;
}