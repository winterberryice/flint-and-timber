#pragma once

#include <SDL3/SDL.h>

#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "PipelineState.h"

class DiligentApp {
public:
    DiligentApp(SDL_Window* window);
    ~DiligentApp();

    void Render();

private:
    void InitializeDiligentEngine(SDL_Window* window);
    void CreatePipelineState();

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pPSO;
};
