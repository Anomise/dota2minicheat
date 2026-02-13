#include "menu.h"
#include "style.h"
#include "../globals.h"
#include "../config.h"
#include "../sdk/interfaces.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <cmath>
#include <cstdio>

static const char* TAB_NAMES[]  = { "CAMERA", "ESP", "AWARENESS", "AUTO", "VISUALS", "INFO", "SETTINGS" };
static const char* TAB_ICONS[]  = { "[C]",    "[E]", "[A]",        "[M]",  "[V]",     "[I]",  "[S]"      };
static int currentTab = 0;

struct Particle { float x, y, vx, vy, alpha, size; };
static std::vector<Particle> particles;
static bool particlesInit = false;

static void InitParticles() {
    particles.resize(40);
    for (auto& p : particles) {
        p.x = (float)(rand() % 600); p.y = (float)(rand() % 500);
        p.vx = ((rand() % 100) / 100.f - 0.5f) * 0.3f;
        p.vy = ((rand() % 100) / 100.f - 0.5f) * 0.3f;
        p.alpha = (rand() % 60 + 20) / 255.f;
        p.size = (rand() % 3) + 1.f;
    }
    particlesInit = true;
}

static void DrawParticles(ImVec2 origin, ImVec2 size) {
    if (!particlesInit) InitParticles();
    auto* dl = ImGui::GetWindowDrawList();
    for (auto& p : particles) {
        p.x += p.vx; p.y += p.vy;
        if (p.x < 0 || p.x > size.x) p.vx = -p.vx;
        if (p.y < 0 || p.y > size.y) p.vy = -p.vy;
        dl->AddCircleFilled(ImVec2(origin.x + p.x, origin.y + p.y), p.size,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, p.alpha)));
    }
}

static void SectionHeader(const char* label) {
    auto accent = Style::GetAccent(Config::Get().menuAccent);
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, accent.primary);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    Style::DrawGlowLine(accent.primary, 2.f);
}

static void HelpTooltip(const char* desc) {
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip(); ImGui::PushTextWrapPos(300.f);
        ImGui::TextUnformatted(desc); ImGui::PopTextWrapPos(); ImGui::EndTooltip();
    }
}

static void TabCamera() {
    auto& c = Config::Get();
    SectionHeader("Camera Hack");
    Style::ToggleButton("Enable Camera Hack", &c.camerHack);
    HelpTooltip("Unlocks camera distance limit");
    if (c.camerHack) {
        ImGui::Indent(10.f);
        ImGui::SliderFloat("Distance", &c.cameraDistance, 800.f, 5000.f, "%.0f");
        ImGui::Text("Presets:"); ImGui::SameLine();
        if (ImGui::SmallButton("Default")) c.cameraDistance = 1200.f; ImGui::SameLine();
        if (ImGui::SmallButton("Medium"))  c.cameraDistance = 1600.f; ImGui::SameLine();
        if (ImGui::SmallButton("Far"))     c.cameraDistance = 2000.f; ImGui::SameLine();
        if (ImGui::SmallButton("Ultra"))   c.cameraDistance = 3000.f;
        ImGui::SliderFloat("FOV Offset", &c.cameraFOV, -30.f, 30.f, "%.1f");
        Style::ToggleButton("Remove Fog", &c.cameraFog);
        ImGui::Unindent(10.f);
    }
}

static void TabESP() {
    auto& c = Config::Get();
    SectionHeader("Enemy ESP");
    Style::ToggleButton("Enable ESP", &c.espEnabled);
    if (c.espEnabled) {
        ImGui::Indent(10.f);
        ImGui::Columns(2, "##espc", false);
        Style::ToggleButton("Health Bars", &c.espHealth);
        Style::ToggleButton("Mana Bars", &c.espMana);
        Style::ToggleButton("Hero Names", &c.espNames);
        ImGui::NextColumn();
        Style::ToggleButton("Hero Icons", &c.espHeroIcons);
        Style::ToggleButton("Spell Tracker", &c.espSpellTracker);
        Style::ToggleButton("Items", &c.espItems);
        ImGui::Columns(1);
        ImGui::Separator();
        Style::ToggleButton("Illusion Detector", &c.espIllusions);
        ImGui::SliderFloat("Font Size", &c.espFontSize, 10.f, 24.f, "%.0f px");
        ImGui::Unindent(10.f);
    }
}

static void TabAwareness() {
    auto& c = Config::Get();
    SectionHeader("Map Awareness");
    Style::ToggleButton("Enable Awareness", &c.awarenessEnabled);
    if (c.awarenessEnabled) {
        ImGui::Indent(10.f);
        Style::ToggleButton("Tower Range", &c.towerRange);
        if (c.towerRange) ImGui::ColorEdit4("Tower Color", c.towerRangeColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        Style::ToggleButton("XP Range", &c.expRange);
        if (c.expRange) ImGui::ColorEdit4("XP Color", c.expRangeColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        Style::ToggleButton("Attack Range", &c.attackRange);
        Style::ToggleButton("Blink Dagger Range", &c.blinkRange);
        ImGui::Separator();
        Style::ToggleButton("Danger Indicator", &c.dangerIndicator);
        Style::ToggleButton("Last Hit Helper", &c.lastHitHelper);
        ImGui::Unindent(10.f);
    }
}

static void TabAuto() {
    auto& c = Config::Get();
    SectionHeader("Automation");
    Style::ToggleButton("Auto Accept Match", &c.autoAccept);
    Style::ToggleButton("Auto Mute Enemies", &c.autoMute);
}

static void TabVisuals() {
    auto& c = Config::Get();
    SectionHeader("Visual Modifications");
    Style::ToggleButton("Weather Hack", &c.weatherHack);
    if (c.weatherHack) {
        const char* w[] = {"Default","Snow","Rain","Moonbeam","Pestilence","Harvest","Sirocco","Spring","Ash"};
        ImGui::Combo("Weather", &c.weatherType, w, IM_ARRAYSIZE(w));
    }
    Style::ToggleButton("Particle Effects", &c.particleHack);
}

static void TabInfo() {
    SectionHeader("Local Hero Info");
    auto localHero = SDK::GetLocalHero();
    if (!localHero || !localHero->IsAlive()) { ImGui::TextDisabled("No hero found or dead."); return; }
    auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(localHero);
    auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(localHero);

    ImGui::BeginChild("##hs", ImVec2(0, 160), true);
    ImGui::Columns(2, "##sc", false);
    ImGui::Text("Name: %s", npc->GetUnitName());
    ImGui::Text("Hero ID: %d  Level: %d", hero->GetHeroID(), npc->GetLevel());
    ImGui::Text("Primary: %s", hero->GetPrimaryAttributeName());
    ImGui::TextColored(ImVec4(0.4f,1.f,0.4f,1.f), "HP: %d/%d (+%.1f)", npc->GetHealth(), npc->GetMaxHealth(), npc->GetHealthRegen());
    ImGui::TextColored(ImVec4(0.4f,0.6f,1.f,1.f), "MP: %.0f/%.0f (+%.1f)", npc->GetMana(), npc->GetMaxMana(), npc->GetManaRegen());
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.f,0.4f,0.4f,1.f), "STR: %.1f", hero->GetStrengthTotal());
    ImGui::TextColored(ImVec4(0.4f,1.f,0.4f,1.f), "AGI: %.1f", hero->GetAgilityTotal());
    ImGui::TextColored(ImVec4(0.4f,0.7f,1.f,1.f), "INT: %.1f", hero->GetIntellectTotal());
    ImGui::Text("MS: %d  AR: %d", npc->GetMoveSpeed(), npc->GetAttackRange());
    ImGui::Columns(1);
    ImGui::EndChild();

    SectionHeader("Status");
    auto Badge = [](const char* l, bool a, ImVec4 c) {
        if (a) { ImGui::PushStyleColor(ImGuiCol_Button, c); ImGui::SmallButton(l); ImGui::PopStyleColor(); }
        else { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.2f,0.25f,1.f));
               ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f,0.4f,0.45f,1.f));
               ImGui::SmallButton(l); ImGui::PopStyleColor(2); }
        ImGui::SameLine();
    };
    Badge("STUN",npc->IsStunned(),{0.9f,0.8f,0.1f,1.f});
    Badge("SIL",npc->IsSilenced(),{0.3f,0.6f,1.f,1.f});
    Badge("HEX",npc->IsHexed(),{0.7f,0.3f,1.f,1.f});
    Badge("ROOT",npc->IsRooted(),{0.3f,0.8f,0.3f,1.f});
    Badge("BKB",npc->IsMagicImmune(),{0.9f,0.7f,0.1f,1.f});
    ImGui::NewLine();

    SectionHeader("Abilities");
    std::vector<AbilityInfo> abs;
    SDK::CollectAbilities(npc, abs);
    if (!abs.empty()) {
        ImGui::BeginTable("##ab", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Lvl",0,30);
        ImGui::TableSetupColumn("CD",0,50); ImGui::TableSetupColumn("Mana",0,40);
        ImGui::TableSetupColumn("Chrg",0,40); ImGui::TableHeadersRow();
        for (auto& a : abs) {
            ImGui::TableNextRow(); ImGui::TableNextColumn();
            if (a.isUltimate) ImGui::TextColored({1,0.85f,0.3f,1}, "%s", a.name.c_str());
            else ImGui::TextUnformatted(a.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%d", a.level);
            ImGui::TableNextColumn();
            if (a.isOnCooldown()) ImGui::TextColored({1,0.4f,0.4f,1},"%.1f",a.cooldown);
            else if (a.level > 0) ImGui::TextColored({0.4f,1,0.4f,1},"Rdy");
            else ImGui::TextDisabled("-");
            ImGui::TableNextColumn(); ImGui::Text("%d", a.manaCost);
            ImGui::TableNextColumn();
            if (a.charges > 0) ImGui::Text("%d", a.charges); else ImGui::TextDisabled("-");
        }
        ImGui::EndTable();
    }
}

static void TabSettings() {
    auto& c = Config::Get();
    SectionHeader("Menu Settings");
    ImGui::SliderFloat("Opacity", &c.menuAlpha, 0.5f, 1.f, "%.2f");
    const char* ac[] = {"Crimson Red","Ocean Blue","Emerald Green","Royal Purple","Sunset Orange"};
    if (ImGui::Combo("Accent", &c.menuAccent, ac, IM_ARRAYSIZE(ac)))
        Style::ApplyModernDark(c.menuAlpha, c.menuAccent);
    Style::ToggleButton("Stream Proof", &c.streamProof);
    SectionHeader("Config");
    if (ImGui::Button("Save Config", ImVec2(-1, 36))) c.Save("dota2cheat.cfg");
    if (ImGui::Button("Load Config", ImVec2(-1, 36))) { c.Load("dota2cheat.cfg"); Style::ApplyModernDark(c.menuAlpha, c.menuAccent); }
    if (ImGui::Button("Reset Default", ImVec2(-1, 36))) { c = Config(); Style::ApplyModernDark(c.menuAlpha, c.menuAccent); }
    SectionHeader("Info");
    ImGui::TextDisabled("Dota 2 Cheat v2.0 | %s %s", __DATE__, __TIME__);
    ImGui::TextDisabled("INSERT=Menu  END=Unload");
    ImGui::TextDisabled("FPS: %.0f", ImGui::GetIO().Framerate);
}

void Menu::Initialize() {
    auto& c = Config::Get();
    Style::ApplyModernDark(c.menuAlpha, c.menuAccent);
    auto& io = ImGui::GetIO();
    const char* fonts[] = {"C:\\Windows\\Fonts\\segoeui.ttf","C:\\Windows\\Fonts\\arial.ttf"};
    for (auto f : fonts) { if (std::filesystem::exists(f)) { ImFontConfig fc; fc.OversampleH=3; fc.OversampleV=2; io.Fonts->AddFontFromFileTTF(f,16.f,&fc); break; } }
}

void Menu::Render() {
    if (!G::bMenuOpen) return;
    auto& cfg = Config::Get();
    auto accent = Style::GetAccent(cfg.menuAccent);

    ImGui::SetNextWindowSize(ImVec2(680, 540), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin("##MW", &G::bMenuOpen, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End(); ImGui::PopStyleVar(); return;
    }
    ImGui::PopStyleVar();

    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();
    auto* dl = ImGui::GetWindowDrawList();

    DrawParticles(wp, ws);

    // Header
    float hH = 52.f;
    dl->AddRectFilled(wp, ImVec2(wp.x+ws.x, wp.y+hH), ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f,0.06f,0.08f,1.f)), 12.f, ImDrawFlags_RoundCornersTop);
    auto lc = ImGui::ColorConvertFloat4ToU32(accent.primary);
    auto lcf = ImGui::ColorConvertFloat4ToU32(ImVec4(accent.primary.x,accent.primary.y,accent.primary.z,0.f));
    dl->AddRectFilledMultiColor(ImVec2(wp.x,wp.y+hH-3), ImVec2(wp.x+ws.x,wp.y+hH), lc, lcf, lcf, lc);
    dl->AddText(ImVec2(wp.x+20, wp.y+16), lc, "DOTA 2 CHEAT");
    dl->AddText(ImVec2(wp.x+ws.x-50, wp.y+18), ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f,0.5f,0.55f,1.f)), "v2.0");
    ImGui::SetCursorPosY(hH);

    float sbW = 140.f;

    // Sidebar
    ImGui::SetCursorPos(ImVec2(0, hH));
    ImGui::BeginChild("##sb", ImVec2(sbW, ws.y - hH), false, ImGuiWindowFlags_NoScrollbar);
    auto sp = ImGui::GetWindowPos();
    dl->AddRectFilled(sp, ImVec2(sp.x+sbW, sp.y+ws.y-hH), ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f,0.06f,0.08f,0.95f)));
    ImGui::Spacing(); ImGui::Spacing();
    for (int i = 0; i < IM_ARRAYSIZE(TAB_NAMES); i++) {
        ImGui::PushID(i);
        bool sel = (currentTab == i);
        char lbl[64]; snprintf(lbl, 64, "  %s  %s", TAB_ICONS[i], TAB_NAMES[i]);
        auto bp = ImGui::GetCursorScreenPos();
        float bH = 36.f, bW = sbW - 16.f;
        ImGui::SetCursorPosX(8.f);
        if (sel) {
            dl->AddRectFilled(ImVec2(bp.x,bp.y), ImVec2(bp.x+3,bp.y+bH), lc, 2.f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(accent.primary.x,accent.primary.y,accent.primary.z,0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.primary.x,accent.primary.y,accent.primary.z,0.25f));
            ImGui::PushStyleColor(ImGuiCol_Text, accent.primary);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.05f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f,0.6f,0.65f,1.f));
        }
        if (ImGui::Button(lbl, ImVec2(bW, bH))) currentTab = i;
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Content
    ImGui::SetCursorPos(ImVec2(sbW, hH));
    ImGui::BeginChild("##ct", ImVec2(ws.x - sbW, ws.y - hH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::SetCursorPos(ImVec2(20, 16));
    switch (currentTab) {
        case 0: TabCamera(); break;
        case 1: TabESP(); break;
        case 2: TabAwareness(); break;
        case 3: TabAuto(); break;
        case 4: TabVisuals(); break;
        case 5: TabInfo(); break;
        case 6: TabSettings(); break;
    }
    ImGui::EndChild();

    ImGui::End();
}

void Menu::Shutdown() {}
