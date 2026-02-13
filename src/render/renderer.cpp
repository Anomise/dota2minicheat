#include "renderer.h"
#include "menu.h"
#include "style.h"

namespace Renderer {
    static ImDrawList* drawList = nullptr;

    bool Initialize(IDXGISwapChain* swapChain) {
        DXGI_SWAP_CHAIN_DESC desc;
        swapChain->GetDesc(&desc);
        G::hWnd = desc.OutputWindow;
        G::pSwapChain = swapChain;
        swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&G::pDevice));
        G::pDevice->GetImmediateContext(&G::pContext);
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        if (backBuffer) { G::pDevice->CreateRenderTargetView(backBuffer, nullptr, &G::pRenderTarget); backBuffer->Release(); }
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        ImGui_ImplWin32_Init(G::hWnd);
        ImGui_ImplDX11_Init(G::pDevice, G::pContext);
        Menu::Initialize();
        RECT rect;
        GetClientRect(G::hWnd, &rect);
        G::screenWidth = static_cast<float>(rect.right - rect.left);
        G::screenHeight = static_cast<float>(rect.bottom - rect.top);
        G::bInitialized = true;
        return true;
    }

    void BeginFrame() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(G::screenWidth, G::screenHeight));
        ImGui::Begin("##overlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        drawList = ImGui::GetWindowDrawList();
        ImGui::End();
    }

    void EndFrame() {
        Menu::Render();
        ImGui::Render();
        G::pContext->OMSetRenderTargets(1, &G::pRenderTarget, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Shutdown() {
        Menu::Shutdown();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (G::pRenderTarget) { G::pRenderTarget->Release(); G::pRenderTarget = nullptr; }
    }

    void DrawFilledRect(float x, float y, float w, float h, ImU32 color) {
        if (!drawList) return;
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
    }
    void DrawRect(float x, float y, float w, float h, ImU32 color, float thickness) {
        if (!drawList) return;
        drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.f, 0, thickness);
    }
    void DrawLine(float x1, float y1, float x2, float y2, ImU32 color, float thickness) {
        if (!drawList) return;
        drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
    }
    void DrawCircle(float x, float y, float radius, ImU32 color, int segments, float thickness) {
        if (!drawList) return;
        drawList->AddCircle(ImVec2(x, y), radius, color, segments, thickness);
    }
    void DrawFilledCircle(float x, float y, float radius, ImU32 color, int segments) {
        if (!drawList) return;
        drawList->AddCircleFilled(ImVec2(x, y), radius, color, segments);
    }
    void DrawText(float x, float y, ImU32 color, const char* text, float fontSize, bool center) {
        if (!drawList || !text) return;
        if (center) { auto size = ImGui::CalcTextSize(text); x -= size.x * 0.5f; }
        drawList->AddText(nullptr, fontSize, ImVec2(x, y), color, text);
    }
    void DrawProgressBar(float x, float y, float w, float h, float percent, ImU32 fgColor, ImU32 bgColor) {
        if (!drawList) return;
        percent = Math::Clamp(percent, 0.f, 1.f);
        drawList->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), IM_COL32(0, 0, 0, 180), 2.f);
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), bgColor, 2.f);
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w * percent, y + h), fgColor, 2.f);
    }
}
