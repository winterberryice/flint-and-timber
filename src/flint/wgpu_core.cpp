#include "wgpu_core.h"

#include <iostream>

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
        std::cout << "OnAdapterReceived called with status: " << status << std::endl;

        AdapterRequestData *data = static_cast<AdapterRequestData *>(userdata1);
        if (status == WGPURequestAdapterStatus_Success)
        {
            std::cout << "Adapter request successful" << std::endl;
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

    // Add this struct after AdapterRequestData
    struct DeviceRequestData
    {
        WGPUDevice device = nullptr;
        bool done = false;
    };

    // Add this callback function
    static void OnDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
    {
        std::cout << "OnDeviceReceived called with status: " << status << std::endl;

        DeviceRequestData *data = static_cast<DeviceRequestData *>(userdata1);
        if (status == WGPURequestDeviceStatus_Success)
        {
            std::cout << "Device request successful" << std::endl;
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

void flint::init_wgpu(
    const uint32_t width,
    const uint32_t height,
    SDL_Window *window,
    WGPUInstance &out_instance,
    WGPUSurface &out_surface,
    WGPUTextureFormat &out_surface_format,
    WGPUAdapter &out_adapter,
    WGPUDevice &out_device,
    WGPUQueue &out_queue //
)
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

    // Request adapter
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.compatibleSurface = nullptr;
    adapterOptions.powerPreference = WGPUPowerPreference_Undefined;
    adapterOptions.backendType = WGPUBackendType_Undefined;
    adapterOptions.forceFallbackAdapter = false;

    AdapterRequestData adapterData;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents; // Changed this!
    callbackInfo.callback = OnAdapterReceived;
    callbackInfo.userdata1 = &adapterData;
    callbackInfo.userdata2 = nullptr;

    std::cout << "Requesting WebGPU adapter..." << std::endl;
    wgpuInstanceRequestAdapter(m_instance, &adapterOptions, callbackInfo);

    std::cout << "Processing events until adapter callback..." << std::endl;
    // Process events until callback is called
    int attempts = 0;
    while (!adapterData.done && attempts < 1000)
    {
        // Try to process pending events
        wgpuInstanceProcessEvents(m_instance);

        attempts++;
        if (attempts % 100 == 0)
        {
            std::cout << "Still waiting... attempt " << attempts << std::endl;
        }
    }

    std::cout << "Adapter callback completed. Done: " << adapterData.done << std::endl;

    if (!adapterData.adapter)
    {
        std::cerr << "No suitable WebGPU adapter found" << std::endl;
        throw std::runtime_error("No suitable WebGPU adapter found");
    }

    auto m_adapter = adapterData.adapter;
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

    DeviceRequestData deviceData;

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.nextInChain = nullptr;
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    deviceCallbackInfo.callback = OnDeviceReceived;
    deviceCallbackInfo.userdata1 = &deviceData;
    deviceCallbackInfo.userdata2 = nullptr;

    std::cout << "Requesting WebGPU device..." << std::endl;
    wgpuAdapterRequestDevice(m_adapter, &deviceDesc, deviceCallbackInfo);

    std::cout << "Processing events until device callback..." << std::endl;
    attempts = 0;
    while (!deviceData.done && attempts < 1000)
    {
        wgpuInstanceProcessEvents(m_instance);
        attempts++;
        if (attempts % 100 == 0)
        {
            std::cout << "Still waiting for device... attempt " << attempts << std::endl;
        }
    }

    std::cout << "Device callback completed. Done: " << deviceData.done << std::endl;

    if (!deviceData.device)
    {
        std::cerr << "Failed to create WebGPU device" << std::endl;
        throw std::runtime_error("Failed to create WebGPU device");
    }

    auto m_device = deviceData.device;
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