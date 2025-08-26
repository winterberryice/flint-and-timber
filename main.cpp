#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <iostream>

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

bool InitWebGPU()
{
    // Create instance
    WGPUInstanceDescriptor instanceDesc = {};
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
    {
        std::cerr << "Failed to create WebGPU instance!" << std::endl;
        return false;
    }

    std::cout << "WebGPU instance created successfully!" << std::endl;
    return true;
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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // Just poll events for now - we'll add rendering later
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main()
{
    Start();
    return 0;
}