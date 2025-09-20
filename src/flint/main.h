#pragma once

#include <sdl3webgpu.h>
#include <webgpu/webgpu.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "block.h"
#include "camera.h"
#include "chunk.h"
#include "vertex.h"
#include "cube_geometry.h"
#include "shader.wgsl.h"

namespace flint
{

    class Application
    {
    public:
        Application();
        ~Application();

        // Runs the application's main loop.
        void run();

    private:
        // --- Initialization ---
        void init();
        void initSdl();
        void initWebGpu();
        void getWgpuDevice();
        void initSwapChain();
        void initDepthTexture();
        void initCameraAndChunk();
        void initPipeline();

        // --- Main Loop ---
        void mainLoop();
        void handleEvents();
        void update();
        void render();

        // --- Teardown ---
        void cleanup();

        // --- Helpers ---
        void resize(int new_width, int new_height);
        void setMouseGrab(bool grab);
        void buildChunkMesh();
        WGPUShaderModule createShaderModule(const std::string &source);
        void processKeyboardInput(const SDL_KeyboardEvent &keyEvent);

        // --- Windowing & App State ---
        SDL_Window *mWindow = nullptr;
        bool mIsRunning = true;
        bool mMouseGrabbed = false;
        glm::uvec2 mSize;

        // --- WebGPU Core Objects ---
        WGPUInstance mInstance = nullptr;
        WGPUSurface mSurface = nullptr;
        WGPUAdapter mAdapter = nullptr;
        WGPUDevice mDevice = nullptr;
        WGPUQueue mQueue = nullptr;

        // --- WebGPU Rendering State ---
        WGPUSurfaceConfiguration mSurfaceConfig{};
        WGPURenderPipeline mRenderPipeline = nullptr;
        WGPUTexture mDepthTexture = nullptr;
        WGPUTextureView mDepthTextureView = nullptr;

        // --- Application Logic Objects ---
        Camera mCamera;
        CameraController mCameraController;
        CameraUniform mCameraUniform;
        Chunk mChunk;

        // --- GPU Buffers ---
        WGPUBuffer mCameraBuffer = nullptr;
        WGPUBindGroup mCameraBindGroup = nullptr;
        WGPUBuffer mChunkVertexBuffer = nullptr;
        WGPUBuffer mChunkIndexBuffer = nullptr;
        uint32_t mNumChunkIndices = 0;
    };

} // namespace flint