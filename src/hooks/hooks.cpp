#include "hooks.h"
#include "input_hook.h"
#include "../render/renderer.h"
#include "../features/feature_manager.h"
#include "../sdk/interfaces.h"

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
static PresentFn oPresent = nullptr;

typedef HRESULT(__stdcall* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static ResizeBuffersFn oResizeBuffers = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static WNDPROC oWndProc = nullptr;

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) { G::bMenuOpen = !G::bMenuOpen; return 0; }
    if (msg == WM_KEYDOWN && wParam == VK_END) { G::bShouldUnload = true; return 0; }
    if (G::bMenuOpen) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)) return 0;
    }
    return CallWindowProcA(oWndProc, hWnd, msg, wParam, lParam);
}

HRESULT __stdcall PresentHook(IDXGISwapChain* sc, UINT si, UINT fl) {
    static bool once = false;
    if (!once) {
        Renderer::Initialize(sc);
        oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(G::hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));
        once = true;
    }
    if (G::bInitialized) { Renderer::BeginFrame(); Features::OnFrame(); Renderer::EndFrame(); }
    return oPresent(sc, si, fl);
}

HRESULT __stdcall ResizeBuffersHook(IDXGISwapChain* sc, UINT bc, UINT w, UINT h, DXGI_FORMAT fmt, UINT fl) {
    if (G::pRenderTarget) { G::pRenderTarget->Release(); G::pRenderTarget = nullptr; }
    auto hr = oResizeBuffers(sc, bc, w, h, fmt, fl);
    ID3D11Texture2D* bb = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb));
    if (bb && G::pDevice) { G::pDevice->CreateRenderTargetView(bb, nullptr, &G::pRenderTarget); bb->Release(); }
    G::screenWidth = (float)w; G::screenHeight = (float)h;
    return hr;
}

static bool GetSwapChainVTable(void**& vtable) {
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0, 0, GetModuleHandle(nullptr),
                     nullptr, nullptr, nullptr, nullptr, L"DX11D", nullptr};
    RegisterClassEx(&wc);
    HWND dw = CreateWindowExA(0, "DX11D", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{}; sd.BufferCount=1; sd.BufferDesc.Width=2; sd.BufferDesc.Height=2;
    sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate={60,1};
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow=dw; sd.SampleDesc.Count=1;
    sd.Windowed=TRUE; sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* dsc=nullptr; ID3D11Device* dd=nullptr; ID3D11DeviceContext* dc=nullptr; D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&sd,&dsc,&dd,&fl,&dc))) {
        DestroyWindow(dw); UnregisterClass(wc.lpszClassName,wc.hInstance); return false;
    }
    vtable = *reinterpret_cast<void***>(dsc);
    dsc->Release(); dd->Release(); dc->Release();
    DestroyWindow(dw); UnregisterClass(wc.lpszClassName,wc.hInstance);
    return true;
}

namespace Hooks {
    bool Initialize() {
        if (MH_Initialize() != MH_OK) return false;
        void** vt = nullptr;
        if (!GetSwapChainVTable(vt)) return false;
        MH_CreateHook(vt[8], &PresentHook, reinterpret_cast<void**>(&oPresent));
        MH_CreateHook(vt[13], &ResizeBuffersHook, reinterpret_cast<void**>(&oResizeBuffers));
        MH_EnableHook(MH_ALL_HOOKS);
        SDK::Initialize();
        Features::Initialize();
        return true;
    }
    void Shutdown() {
        Features::Shutdown();
        MH_DisableHook(MH_ALL_HOOKS); MH_RemoveHook(MH_ALL_HOOKS); MH_Uninitialize();
        if (oWndProc && G::hWnd) SetWindowLongPtrA(G::hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
        Renderer::Shutdown();
    }
}
