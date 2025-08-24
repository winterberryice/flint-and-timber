#include "FlintAndTimberApp.hpp"

#include <SDL3/SDL_properties.h>

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
#    include "DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#elif PLATFORM_MACOS
#    include "DiligentCore/Graphics/GraphicsEngineMetal/interface/EngineFactoryMetal.h"
#endif

namespace Diligent
{
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

FlintAndTimberApp::FlintAndTimberApp(SDL_Window* window)
{
    InitializeDiligentEngine(window);
    CreatePipelineState();
}

FlintAndTimberApp::~FlintAndTimberApp()
{
    m_pImmediateContext->Flush();
}

void FlintAndTimberApp::InitializeDiligentEngine(SDL_Window* window)
{
    Diligent::SwapChainDesc SCDesc;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);

#if PLATFORM_WIN32
    Diligent::IEngineFactoryD3D11* pFactoryD3D11 = Diligent::GetEngineFactoryD3D11();
    Diligent::EngineD3D11CreateInfo EngineCI;
    pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
    void* hWnd = SDL_GetPointerProperty(props, "SDL.window.win32.hwnd", NULL);
    Diligent::Win32NativeWindow Window{hWnd};
    pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, Diligent::FullScreenModeDesc{}, Window, &m_pSwapChain);
#elif PLATFORM_LINUX
    Diligent::IEngineFactoryVk* pFactoryVk = Diligent::GetEngineFactoryVk();
    Diligent::EngineVkCreateInfo EngineCI;
    pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
    if(m_pDevice)
    {
        Diligent::LinuxNativeWindow LinuxWindow;
        if (strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)
        {
            LinuxWindow.pDisplay = SDL_GetPointerProperty(props, "SDL.window.x11.display", NULL);
            LinuxWindow.WindowId = SDL_GetNumberProperty(props, "SDL.window.x11.window", 0);
        }
        else
        {
            LinuxWindow.pDisplay = SDL_GetPointerProperty(props, "SDL.window.wayland.display", NULL);
            LinuxWindow.WindowId = (uint64_t)(uintptr_t)SDL_GetPointerProperty(props, "SDL.window.wayland.surface", NULL);
        }
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, LinuxWindow, &m_pSwapChain);
    }
#elif PLATFORM_MACOS
    Diligent::IEngineFactoryMtl* pFactoryMtl = Diligent::GetEngineFactoryMtl();
    Diligent::EngineMtlCreateInfo EngineCI;
    pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &m_pDevice, &m_pImmediateContext);
    void* pNSWindow = SDL_GetPointerProperty(props, "SDL.window.cocoa.window", NULL);
    Diligent::MacOSNativeWindow Window{pNSWindow};
    pFactoryMtl->CreateSwapChainMtl(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
#endif
}

void FlintAndTimberApp::CreatePipelineState()
{
    Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.Name = "Flint & Timber PSO";
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

void FlintAndTimberApp::Render()
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
