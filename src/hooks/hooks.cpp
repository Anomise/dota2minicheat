#include "hooks.h"
#include "input_hook.h"
#include "../render/renderer.h"
#include "../features/feature_manager.h"
#include "../sdk/interfaces.h"

typedef HRESULT(__stdcall*PresentFn)(IDXGISwapChain*,UINT,UINT);
static PresentFn oPresent=nullptr;
typedef HRESULT(__stdcall*ResizeFn)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
static ResizeFn oResize=nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
static WNDPROC oWndProc=nullptr;

LRESULT CALLBACK WPHook(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(m==WM_KEYDOWN&&w==VK_INSERT){G::bMenuOpen=!G::bMenuOpen;return 0;}
    if(m==WM_KEYDOWN&&w==VK_END){G::bShouldUnload=true;return 0;}
    if(G::bMenuOpen){ImGui_ImplWin32_WndProcHandler(h,m,w,l);
        if((m>=WM_MOUSEFIRST&&m<=WM_MOUSELAST)||(m>=WM_KEYFIRST&&m<=WM_KEYLAST))return 0;}
    return CallWindowProcA(oWndProc,h,m,w,l);
}

HRESULT __stdcall PHook(IDXGISwapChain*sc,UINT si,UINT fl) {
    static bool once=false;
    if(!once){Renderer::Initialize(sc);
        oWndProc=(WNDPROC)SetWindowLongPtrA(G::hWnd,GWLP_WNDPROC,(LONG_PTR)WPHook);once=true;}
    if(G::bInitialized){Renderer::BeginFrame();Features::OnFrame();Renderer::EndFrame();}
    return oPresent(sc,si,fl);
}

HRESULT __stdcall RHook(IDXGISwapChain*sc,UINT bc,UINT w,UINT h,DXGI_FORMAT f,UINT fl) {
    if(G::pRenderTarget){G::pRenderTarget->Release();G::pRenderTarget=nullptr;}
    auto hr=oResize(sc,bc,w,h,f,fl);
    ID3D11Texture2D*bb=nullptr;sc->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&bb);
    if(bb&&G::pDevice){G::pDevice->CreateRenderTargetView(bb,nullptr,&G::pRenderTarget);bb->Release();}
    G::screenWidth=(float)w;G::screenHeight=(float)h;return hr;
}

static bool GetVT(void**&vt) {
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "DXDummy";
    RegisterClassExA(&wc);

    HWND dw = CreateWindowExA(0, "DXDummy", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount=1;
    sd.BufferDesc.Width=2;
    sd.BufferDesc.Height=2;
    sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate={60,1};
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow=dw;
    sd.SampleDesc.Count=1;
    sd.Windowed=TRUE;
    sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain*ds=nullptr;
    ID3D11Device*dd=nullptr;
    ID3D11DeviceContext*dc=nullptr;
    D3D_FEATURE_LEVEL fl;

    if(FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &ds, &dd, &fl, &dc)))
    {
        DestroyWindow(dw);
        UnregisterClassA("DXDummy", wc.hInstance);
        return false;
    }

    vt=*(void***)ds;
    ds->Release();
    dd->Release();
    dc->Release();
    DestroyWindow(dw);
    UnregisterClassA("DXDummy", wc.hInstance);
    return true;
}

namespace Hooks {
    bool Initialize() {
        if(MH_Initialize()!=MH_OK)return false;
        void**vt=nullptr;
        if(!GetVT(vt))return false;
        MH_CreateHook(vt[8],&PHook,(void**)&oPresent);
        MH_CreateHook(vt[13],&RHook,(void**)&oResize);
        MH_EnableHook(MH_ALL_HOOKS);
        SDK::Initialize();
        Features::Initialize();
        return true;
    }
    void Shutdown() {
        Features::Shutdown();
        MH_DisableHook(MH_ALL_HOOKS);
        MH_RemoveHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        if(oWndProc&&G::hWnd)SetWindowLongPtrA(G::hWnd,GWLP_WNDPROC,(LONG_PTR)oWndProc);
        Renderer::Shutdown();
    }
}
