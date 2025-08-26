#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <iostream>
#include <cstring>

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUDevice device = nullptr;
WGPUQueue queue = nullptr;

// Global variables for callbacks
bool adapterReady = false;
bool deviceReady = false;

// Helper function to create WGPUStringView from C string
WGPUStringView makeStringView(const char *str)
{
    WGPUStringView view = {};
    if (str)
    {
        view.data = str;
        view.length = strlen(str);
    }
    return view;
}

// Simple callback for adapter request
void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapterResult, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        adapter = adapterResult;
        std::cout << "Adapter obtained successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to get adapter" << std::endl;
    }
    adapterReady = true;
}

// Simple callback for device request
void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice deviceResult, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        device = deviceResult;
        std::cout << "Device obtained successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to get device" << std::endl;
    }
    deviceReady = true;
}

bool InitWebGPU()
{
    // Create instance
    WGPUInstanceDescriptor instanceDesc = {};
    instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        return false;
    }
    std::cout << "WebGPU instance created successfully!" << std::endl;

    // Request adapter
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = nullptr;
    callbackInfo.userdata2 = nullptr;

    WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapterOpts, callbackInfo);

    // Simple polling for adapter
    while (!adapterReady)
    {
        wgpuInstanceProcessEvents(instance);
    }

    if (!adapter)
    {
        std::cerr << "Failed to obtain adapter!" << std::endl;
        return false;
    }

    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = makeStringView("Primary Device");

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallbackInfo.callback = onDeviceRequestEnded;
    deviceCallbackInfo.userdata1 = nullptr;
    deviceCallbackInfo.userdata2 = nullptr;

    WGPUFuture deviceFuture = wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);

    // Simple polling for device
    while (!deviceReady)
    {
        wgpuInstanceProcessEvents(instance);
    }

    if (!device)
    {
        std::cerr << "Failed to obtain device!" << std::endl;
        return false;
    }

    // Get the queue
    queue = wgpuDeviceGetQueue(device);
    if (!queue)
    {
        std::cerr << "Failed to get device queue!" << std::endl;
        return false;
    }

    std::cout << "WebGPU setup complete!" << std::endl;
    return true;
}

void Cleanup()
{
    if (queue)
        wgpuQueueRelease(queue);
    if (device)
        wgpuDeviceRelease(device);
    if (adapter)
        wgpuAdapterRelease(adapter);
    if (instance)
        wgpuInstanceRelease(instance);
}

void Start()
{
    if (!glfwInit())
    {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(kWidth, kHeight, "WebGPU window", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return;
    }

    // Initialize WebGPU
    if (!InitWebGPU())
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    std::cout << "Starting render loop..." << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Basic command buffer submission (empty for now)
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = makeStringView("Frame Command Encoder");

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = makeStringView("Frame Command Buffer");

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);

        wgpuQueueSubmit(queue, 1, &commandBuffer);

        // Clean up frame resources
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(encoder);

        // Process WebGPU events
        wgpuInstanceProcessEvents(instance);
    }

    Cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Application finished." << std::endl;
}

int main()
{
    Start();
    return 0;
}
