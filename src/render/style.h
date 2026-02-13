#pragma once
#include "imgui.h"
#include <cmath>

namespace Style {
    struct AccentColors {
        ImVec4 primary, primaryHover, primaryActive, header, headerHover, accent;
    };

    inline AccentColors GetAccent(int idx) {
        switch (idx) {
        case 0: return {{0.86f,0.20f,0.27f,1.f},{0.92f,0.30f,0.35f,1.f},{0.76f,0.15f,0.22f,1.f},{0.86f,0.20f,0.27f,0.50f},{0.86f,0.20f,0.27f,0.75f},{0.86f,0.20f,0.27f,1.f}};
        case 1: return {{0.20f,0.45f,0.86f,1.f},{0.30f,0.55f,0.92f,1.f},{0.15f,0.38f,0.76f,1.f},{0.20f,0.45f,0.86f,0.50f},{0.20f,0.45f,0.86f,0.75f},{0.20f,0.45f,0.86f,1.f}};
        case 2: return {{0.18f,0.80f,0.44f,1.f},{0.28f,0.88f,0.52f,1.f},{0.14f,0.68f,0.36f,1.f},{0.18f,0.80f,0.44f,0.50f},{0.18f,0.80f,0.44f,0.75f},{0.18f,0.80f,0.44f,1.f}};
        case 3: return {{0.60f,0.25f,0.86f,1.f},{0.68f,0.35f,0.92f,1.f},{0.52f,0.18f,0.76f,1.f},{0.60f,0.25f,0.86f,0.50f},{0.60f,0.25f,0.86f,0.75f},{0.60f,0.25f,0.86f,1.f}};
        default: return {{0.95f,0.55f,0.15f,1.f},{1.00f,0.65f,0.25f,1.f},{0.85f,0.45f,0.10f,1.f},{0.95f,0.55f,0.15f,0.50f},{0.95f,0.55f,0.15f,0.75f},{0.95f,0.55f,0.15f,1.f}};
        }
    }

    inline void ApplyModernDark(float alpha = 0.95f, int accentIdx = 0) {
        auto& style = ImGui::GetStyle();
        auto* colors = style.Colors;
        auto accent = GetAccent(accentIdx);

        style.WindowPadding = ImVec2(16, 16);
        style.FramePadding = ImVec2(10, 6);
        style.CellPadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(10, 8);
        style.ItemInnerSpacing = ImVec2(8, 4);
        style.ScrollbarSize = 12.f;
        style.GrabMinSize = 12.f;
        style.WindowRounding = 12.f;
        style.ChildRounding = 8.f;
        style.FrameRounding = 6.f;
        style.PopupRounding = 8.f;
        style.ScrollbarRounding = 8.f;
        style.GrabRounding = 4.f;
        style.TabRounding = 6.f;
        style.WindowBorderSize = 1.f;
        style.ChildBorderSize = 1.f;
        style.FrameBorderSize = 0.f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

        ImVec4 bg = ImVec4(0.08f, 0.08f, 0.10f, alpha);
        ImVec4 bgFrame = ImVec4(0.14f, 0.14f, 0.18f, 1.f);
        ImVec4 border = ImVec4(0.22f, 0.22f, 0.28f, 0.60f);

        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.62f, 1.f);
        colors[ImGuiCol_WindowBg] = bg;
        colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.13f, alpha);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.13f, alpha);
        colors[ImGuiCol_Border] = border;
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg] = bgFrame;
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.24f, 1.f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.28f, 1.f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.08f, 1.f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.08f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.5f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.32f, 1.f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.42f, 1.f);
        colors[ImGuiCol_ScrollbarGrabActive] = accent.primary;
        colors[ImGuiCol_CheckMark] = accent.primary;
        colors[ImGuiCol_SliderGrab] = accent.primary;
        colors[ImGuiCol_SliderGrabActive] = accent.primaryHover;
        colors[ImGuiCol_Button] = bgFrame;
        colors[ImGuiCol_ButtonHovered] = accent.header;
        colors[ImGuiCol_ButtonActive] = accent.primaryActive;
        colors[ImGuiCol_Header] = accent.header;
        colors[ImGuiCol_HeaderHovered] = accent.headerHover;
        colors[ImGuiCol_HeaderActive] = accent.primary;
        colors[ImGuiCol_Separator] = border;
        colors[ImGuiCol_SeparatorHovered] = accent.primary;
        colors[ImGuiCol_SeparatorActive] = accent.primaryActive;
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.32f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = accent.header;
        colors[ImGuiCol_ResizeGripActive] = accent.primary;
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.13f, 1.f);
        colors[ImGuiCol_TabHovered] = accent.headerHover;
        colors[ImGuiCol_TabActive] = accent.header;
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.13f, 1.f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.f);
        colors[ImGuiCol_TableBorderStrong] = border;
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.18f, 0.18f, 0.24f, 0.40f);
        colors[ImGuiCol_TextSelectedBg] = accent.header;
        colors[ImGuiCol_DragDropTarget] = accent.primary;
        colors[ImGuiCol_NavHighlight] = accent.primary;
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.60f);
    }

    inline void DrawGlowLine(ImVec4 color, float thickness = 2.f) {
        auto* dl = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        auto width = ImGui::GetContentRegionAvail().x;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        ImU32 colFade = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.f));
        dl->AddRectFilledMultiColor(ImVec2(pos.x, pos.y), ImVec2(pos.x + width, pos.y + thickness), col, col, colFade, colFade);
        ImGui::Dummy(ImVec2(width, thickness + 4.f));
    }

    inline bool ToggleButton(const char* label, bool* v) {
        ImGui::PushID(label);
        auto* dl = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        float height = ImGui::GetFrameHeight() * 0.8f;
        float width = height * 1.8f;
        float radius = height * 0.5f;
        bool changed = false;
        ImGui::InvisibleButton(label, ImVec2(width + ImGui::CalcTextSize(label).x + 8.f, height));
        if (ImGui::IsItemClicked()) { *v = !*v; changed = true; }
        float t = *v ? 1.0f : 0.0f;
        ImU32 bgColor = *v ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.72f, 0.40f, 1.f))
                           : ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.25f, 0.32f, 1.f));
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgColor, radius);
        float knobX = pos.x + radius + t * (width - height);
        dl->AddCircleFilled(ImVec2(knobX, pos.y + radius), radius - 2.f, IM_COL32(240, 240, 245, 255));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::CalcTextSize(label).x);
        ImGui::TextUnformatted(label);
        ImGui::PopID();
        return changed;
    }
}
