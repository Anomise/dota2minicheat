#pragma once
#include "../globals.h"
#include "../utils/math.h"

namespace Renderer {
    bool Initialize(IDXGISwapChain* swapChain);
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    void DrawFilledRect(float x, float y, float w, float h, ImU32 color);
    void DrawRect(float x, float y, float w, float h, ImU32 color, float thickness = 1.f);
    void DrawLine(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.f);
    void DrawCircle(float x, float y, float radius, ImU32 color, int segments = 64, float thickness = 1.f);
    void DrawFilledCircle(float x, float y, float radius, ImU32 color, int segments = 64);
    void DrawText(float x, float y, ImU32 color, const char* text, float fontSize = 14.f, bool center = false);
    void DrawProgressBar(float x, float y, float w, float h, float percent, ImU32 fgColor, ImU32 bgColor);
}
