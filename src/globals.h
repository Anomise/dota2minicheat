#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "MinHook.h"

namespace G {
    inline HMODULE           hModule          = nullptr;
    inline HWND              hWnd             = nullptr;
    inline bool              bShouldUnload    = false;
    inline bool              bMenuOpen        = true;
    inline bool              bInitialized     = false;

    inline ID3D11Device*           pDevice        = nullptr;
    inline ID3D11DeviceContext*    pContext       = nullptr;
    inline IDXGISwapChain*         pSwapChain    = nullptr;
    inline ID3D11RenderTargetView* pRenderTarget = nullptr;

    inline float screenWidth  = 1920.f;
    inline float screenHeight = 1080.f;
}
