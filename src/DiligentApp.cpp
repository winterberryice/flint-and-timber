#include "DiligentApp.hpp"

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"

#if DILIGENT_EXCEPTIONS_ENABLED
#    define VERIFY_EX(expr, msg) \
        do                         \
        {                          \
            if (!(expr))           \
                throw std::runtime_error(msg); \
        } while (false)
#else
#    define VERIFY_EX(expr, msg) Diligent::VERIFY(expr, msg)
#endif

#if PLATFORM_WIN32
#    include "Windows.h"
#    include "DiligentCore/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#    include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#elif PLATFORM_LINUX
#    include "DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVulkan.h"
#elif PLATFORM_MACOS
#    include "DiligentCore/Graphics/GraphicsEngineMetal/interface/EngineFactoryMetal.h"
#endif

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiImplSDL3.h"
#include <SDL_syswm.h>

namespace Diligent
{
#if PLATFORM_WIN32
void* GetWin32WindowHandle(SDL_Window* pWindow)
{
    SDL_SysWMinfo wmInfo;
    SDL_GetWindowWMInfo(pWindow, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
    return wmInfo.info.win.window;
}
#endif

#if PLATFORM_LINUX
void* GetX11WindowHandle(SDL_Window* pWindow)
{
    SDL_SysWMinfo wmInfo;
    SDL_GetWindowWMInfo(pWindow, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
    return reinterpret_cast<void*>(wmInfo.info.x11.window);
}
Display* GetX11Display(SDL_Window* pWindow)
{
    SDL_SysWMinfo wmInfo;
    SDL_GetWindowWMInfo(pWindow, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
    return wmInfo.info.x11.display;
}
#endif

#if PLATFORM_MACOS
void* GetMetalLayer(SDL_Window* pWindow)
{
    SDL_SysWMinfo wmInfo;
    SDL_GetWindowWMInfo(pWindow, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
    return wmInfo.info.cocoa.window;
}
#endif
} // namespace Diligent


static const char* VSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    float4 Pos[3];
    Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
    Pos[1] = float4( 0.0, +0.5, 0.0, 1.0);
    Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(1.0, 0.0, 0.0); // red
    Col[1] = float3(0.0, 1.0, 0.0); // green
    Col[2] = float3(0.0, 0.0, 1.0); // blue

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)";

static const char* PSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)";

DiligentApp::DiligentApp(SDL_Window* window)
{
    InitializeDiligentEngine(window);
    CreatePipelineState();
}

DiligentApp::~DiligentApp()
{
    m_pImmediateContext->Flush();
}

void DiligentApp::InitializeDiligentEngine(SDL_Window* window)
{
    Diligent::SwapChainDesc SCDesc;

#if PLATFORM_WIN32
    Diligent::IEngineFactoryD3D11* pFactoryD3D11 = Diligent::GetEngineFactoryD3D11();
    Diligent::EngineD3D11CreateInfo EngineCI;
    pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
    Diligent::Win32NativeWindow Window{Diligent::GetWin32WindowHandle(window)};
    pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, Diligent::FullScreenModeDesc{}, Window, &m_pSwapChain);
#elif PLATFORM_LINUX
    Diligent::IEngineFactoryVk* pFactoryVk = Diligent::GetEngineFactoryVk();
    Diligent::EngineVkCreateInfo EngineCI;
    pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
    if(m_pDevice)
    {
        SDL_SysWMinfo wmInfo;
        SDL_GetWindowWMInfo(window, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
        Diligent::LinuxNativeWindow LinuxWindow;
        LinuxWindow.pDisplay = wmInfo.info.x11.display;
        LinuxWindow.WindowId = wmInfo.info.x11.window;
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, LinuxWindow, &m_pSwapChain);
    }
#elif PLATFORM_MACOS
    Diligent::IEngineFactoryMtl* pFactoryMtl = Diligent::GetEngineFactoryMtl();
    Diligent::EngineMtlCreateInfo EngineCI;
    pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &m_pDevice, &m_pImmediateContext);
    Diligent::MacOSNativeWindow Window{Diligent::GetMetalLayer(window)};
    pFactoryMtl->CreateSwapChainMtl(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
#endif
}

void DiligentApp::CreatePipelineState()
{
    Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Simple triangle PSO";
    PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint = "main";
        ShaderCI.Desc.Name = "Triangle vertex shader";
        ShaderCI.Source = VSSource;
        m_pDevice->CreateShader(ShaderCI, &pVS);
    }

    Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint = "main";
        ShaderCI.Desc.Name = "Triangle pixel shader";
        ShaderCI.Source = PSSource;
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
}

void DiligentApp::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();

    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_pImmediateContext->SetPipelineState(m_pPSO);

    Diligent::DrawAttribs drawAttrs;
    drawAttrs.NumVertices = 3;
    m_pImmediateContext->Draw(drawAttrs);

    m_pSwapChain->Present();
}
