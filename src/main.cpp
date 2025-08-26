#include <iostream>

// SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_syswm.h>

// The Forge
#include "Common_3/Application/Interfaces/IApp.h"
#include "Common_3/Graphics/Interfaces/IGraphics.h"
#include "Common_3/OS/Interfaces/ILog.h"
#include "Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Common_3/Utilities/Interfaces/IMemory.h"
#include "Common_3/Utilities/Math/MathTypes.h"

// Globals
Renderer* pRenderer = NULL;
Queue* pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};
SwapChain* pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;
Semaphore* pImageAcquiredSemaphore = NULL;

RootSignature* pRootSignature = NULL;
Shader* pTriangleShader = NULL;
Pipeline* pTrianglePipeline = NULL;
Buffer* pTriangleVertexBuffer = NULL;

const uint32_t gDataBufferCount = 2;
uint32_t gFrameIndex = 0;

struct Vertex
{
    float3 mPosition;
    float4 mColor;
};

// Function to get the native window handle from SDL
static WindowHandle get_native_window_handle(SDL_Window* window)
{
    WindowHandle handle = {};
    SDL_SysWMinfo wmInfo;
    SDL_GetWindowWMInfo(window, &wmInfo, SDL_SYSWM_CURRENT_VERSION);

#if defined(SDL_PLATFORM_WINDOWS)
    handle.hwnd = wmInfo.info.win.window;
#elif defined(SDL_PLATFORM_LINUX)
#if defined(SDL_VIDEO_DRIVER_X11)
    handle.display = wmInfo.info.x11.display;
    handle.window = wmInfo.info.x11.window;
#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
    handle.display = wmInfo.info.wl.display;
    handle.surface = wmInfo.info.wl.surface;
#endif
#elif defined(SDL_PLATFORM_APPLE)
    handle.pLayer = wmInfo.info.cocoa.window;
#endif

    return handle;
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Flint & Timber", 800, 600, SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 1. Initialize The Forge Renderer
    RendererDesc settings = {};
    initRenderer("Flint & Timber", &settings, &pRenderer);
    if (!pRenderer)
    {
        std::cerr << "Error initializing The Forge renderer" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 2. Initialize command queue and ring
    QueueDesc queueDesc = {};
    queueDesc.mType = QUEUE_TYPE_GRAPHICS;
    initQueue(pRenderer, &queueDesc, &pGraphicsQueue);

    GpuCmdRingDesc cmdRingDesc = {};
    cmdRingDesc.pQueue = pGraphicsQueue;
    cmdRingDesc.mPoolCount = gDataBufferCount;
    cmdRingDesc.mCmdPerPoolCount = 1;
    cmdRingDesc.mAddSyncPrimitives = true;
    initGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

    initSemaphore(pRenderer, &pImageAcquiredSemaphore);

    // 3. Create Swapchain
    WindowHandle windowHandle = get_native_window_handle(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle = windowHandle;
    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &pGraphicsQueue;
    swapChainDesc.mWidth = width;
    swapChainDesc.mHeight = height;
    swapChainDesc.mImageCount = 3;
    swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
    swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
    swapChainDesc.mEnableVsync = true;
    addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

    if (!pSwapChain)
    {
        std::cerr << "Error creating swapchain" << std::endl;
        // Cleanup Forge
        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        exitSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitQueue(pRenderer, pGraphicsQueue);
        exitRenderer(pRenderer);
        // Cleanup SDL
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 4. Add Depth Buffer
    RenderTargetDesc depthRT = {};
    depthRT.mArraySize = 1;
    depthRT.mClearValue.depth = 0.0f;
    depthRT.mClearValue.stencil = 0;
    depthRT.mDepth = 1;
    depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
    depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
    depthRT.mHeight = height;
    depthRT.mSampleCount = SAMPLE_COUNT_1;
    depthRT.mSampleQuality = 0;
    depthRT.mWidth = width;
    addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

    if (!pDepthBuffer)
    {
        std::cerr << "Error creating depth buffer" << std::endl;
        // Cleanup Forge and SDL
        removeSwapChain(pRenderer, pSwapChain);
        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        exitSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitQueue(pRenderer, pGraphicsQueue);
        exitRenderer(pRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    initResourceLoaderInterface(pRenderer);

    // Create resources for the triangle
    // 1. Create root signature
    RootSignatureDesc rootDesc = {};
    initRootSignature(pRenderer, &rootDesc, &pRootSignature);

    // 2. Create shaders
    ShaderLoadDesc shaderLoadDesc = {};
    shaderLoadDesc.mVert.pFileName = "triangle.vert";
    shaderLoadDesc.mFrag.pFileName = "triangle.frag";
    addShader(pRenderer, &shaderLoadDesc, &pTriangleShader);

    // 3. Create vertex layout and pipeline
    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
    vertexLayout.mAttribCount = 2;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;
    vertexLayout.mAttribs[1].mSemantic = SEMANTIC_COLOR;
    vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.mAttribs[1].mBinding = 0;
    vertexLayout.mAttribs[1].mLocation = 1;
    vertexLayout.mAttribs[1].mOffset = offsetof(Vertex, mColor);

    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = true;
    depthStateDesc.mDepthWrite = true;
    depthStateDesc.mDepthFunc = CMP_LEQUAL;

    PipelineDesc pipelineDesc = {};
    pipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
    pipelineDesc.mGraphicsDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineDesc.mGraphicsDesc.mRenderTargetCount = 1;
    pipelineDesc.mGraphicsDesc.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineDesc.mGraphicsDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineDesc.mGraphicsDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineDesc.mGraphicsDesc.mDepthStencilFormat = pDepthBuffer->mFormat;
    pipelineDesc.mGraphicsDesc.pRootSignature = pRootSignature;
    pipelineDesc.mGraphicsDesc.pShaderProgram = pTriangleShader;
    pipelineDesc.mGraphicsDesc.pVertexLayout = &vertexLayout;
    pipelineDesc.mGraphicsDesc.pRasterizerState = &rasterizerStateDesc;
    pipelineDesc.mGraphicsDesc.pDepthState = &depthStateDesc;
    addPipeline(pRenderer, &pipelineDesc, &pTrianglePipeline);

    // 4. Create vertex buffer
    Vertex triangleVertices[] = {
        { { 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f} },
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f} }
    };

    BufferLoadDesc vbDesc = {};
    vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    vbDesc.mDesc.mSize = sizeof(triangleVertices);
    vbDesc.pData = triangleVertices;
    vbDesc.ppBuffer = &pTriangleVertexBuffer;
    addResource(&vbDesc, NULL);

    waitForAllResourceLoads();

    bool is_running = true;
    SDL_Event event;

    while (is_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                is_running = false;
            }
        }

        // Drawing
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);

        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

        // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
        FenceStatus fenceStatus;
        getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &elem.pFence);

        // Reset cmd pool for this frame
        resetCmdPool(pRenderer, elem.pCmdPool);

        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        RenderTargetBarrier barriers[] = {
            { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
        };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

        // simply record the screen cleaning command
        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        // Draw the triangle
        const uint32_t vbStride = sizeof(Vertex);
        cmdBindPipeline(cmd, pTrianglePipeline);
        cmdBindVertexBuffer(cmd, 1, &pTriangleVertexBuffer, &vbStride, NULL);
        cmdDraw(cmd, 3, 0);

        cmdBindRenderTargets(cmd, NULL);

        barriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

        endCmd(cmd);

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = 1;
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = (uint8_t)swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &elem.pSemaphore;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
    }

    // Cleanup
    waitQueueIdle(pGraphicsQueue);

    removeResource(pTriangleVertexBuffer);
    removePipeline(pRenderer, pTrianglePipeline);
    removeShader(pRenderer, pTriangleShader);
    removeRootSignature(pRenderer, pRootSignature);

    removeRenderTarget(pRenderer, pDepthBuffer);
    removeSwapChain(pRenderer, pSwapChain);

    exitResourceLoaderInterface(pRenderer);
    exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    exitSemaphore(pRenderer, pImageAcquiredSemaphore);
    exitQueue(pRenderer, pGraphicsQueue);
    exitRenderer(pRenderer);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
