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
#include <cstring>

static const char* TABS[]  = {"CAMERA","ESP","AWARENESS","AUTO","VISUALS","INFO","SETTINGS"};
static const char* ICONS[] = {"[C]","[E]","[A]","[M]","[V]","[I]","[S]"};
static int curTab = 0;

// ============================================
// Animated particles
// ============================================
struct Particle { float x, y, vx, vy, a, s; };
static std::vector<Particle> particles;
static bool pInit = false;

static void InitP() {
    particles.resize(40);
    for (auto& p : particles) {
        p.x  = (float)(rand() % 600);
        p.y  = (float)(rand() % 500);
        p.vx = ((rand() % 100) / 100.f - 0.5f) * 0.3f;
        p.vy = ((rand() % 100) / 100.f - 0.5f) * 0.3f;
        p.a  = (rand() % 60 + 20) / 255.f;
        p.s  = (float)(rand() % 3 + 1);
    }
    pInit = true;
}

static void DrawP(ImVec2 o, ImVec2 sz) {
    if (!pInit) InitP();
    auto* dl = ImGui::GetWindowDrawList();
    for (auto& p : particles) {
        p.x += p.vx; p.y += p.vy;
        if (p.x < 0 || p.x > sz.x) p.vx = -p.vx;
        if (p.y < 0 || p.y > sz.y) p.vy = -p.vy;
        dl->AddCircleFilled(
            ImVec2(o.x + p.x, o.y + p.y), p.s,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, p.a))
        );
    }
}

// ============================================
// Helpers
// ============================================
static void Sec(const char* l) {
    auto a = Style::GetAccent(Config::Get().menuAccent);
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, a.primary);
    ImGui::TextUnformatted(l);
    ImGui::PopStyleColor();
    Style::DrawGlowLine(a.primary, 2);
}

static void Tip(const char* d) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(300);
        ImGui::TextUnformatted(d);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// ============================================
// Tab: Camera
// ============================================
static void TabCamera() {
    auto& c = Config::Get();
    Sec("Camera Hack");
    Style::ToggleButton("Enable Camera Hack", &c.camerHack);
    Tip("Unlocks camera distance limit");

    if (c.camerHack) {
        ImGui::Indent(10);
        ImGui::SliderFloat("Distance", &c.cameraDistance, 800, 5000, "%.0f");

        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Default")) c.cameraDistance = 1200;
        ImGui::SameLine();
        if (ImGui::SmallButton("Medium"))  c.cameraDistance = 1600;
        ImGui::SameLine();
        if (ImGui::SmallButton("Far"))     c.cameraDistance = 2000;
        ImGui::SameLine();
        if (ImGui::SmallButton("Ultra"))   c.cameraDistance = 3000;

        ImGui::SliderFloat("FOV Offset", &c.cameraFOV, -30, 30, "%.1f");
        Style::ToggleButton("Remove Fog", &c.cameraFog);
        Tip("Removes fog rendering (client-side only)");
        ImGui::Unindent(10);
    }
}

// ============================================
// Tab: ESP
// ============================================
static void TabESP() {
    auto& c = Config::Get();
    Sec("Enemy ESP");
    Style::ToggleButton("Enable ESP", &c.espEnabled);

    if (c.espEnabled) {
        ImGui::Indent(10);
        ImGui::Columns(2, "##ec", false);

        Style::ToggleButton("Health Bars", &c.espHealth);
        Style::ToggleButton("Mana Bars", &c.espMana);
        Style::ToggleButton("Hero Names", &c.espNames);

        ImGui::NextColumn();

        Style::ToggleButton("Spell Tracker", &c.espSpellTracker);
        Style::ToggleButton("Items", &c.espItems);
        Style::ToggleButton("Illusion Detect", &c.espIllusions);

        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SliderFloat("Font Size", &c.espFontSize, 10, 24, "%.0f px");
        ImGui::Unindent(10);
    }
}

// ============================================
// Tab: Awareness
// ============================================
static void TabAwareness() {
    auto& c = Config::Get();
    Sec("Map Awareness");
    Style::ToggleButton("Enable Awareness", &c.awarenessEnabled);

    if (c.awarenessEnabled) {
        ImGui::Indent(10);

        Style::ToggleButton("Tower Range", &c.towerRange);
        if (c.towerRange)
            ImGui::ColorEdit4("Tower Color", c.towerRangeColor,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

        Style::ToggleButton("XP Range", &c.expRange);
        if (c.expRange)
            ImGui::ColorEdit4("XP Color", c.expRangeColor,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

        Style::ToggleButton("Attack Range", &c.attackRange);
        Tip("Shows your hero's attack range circle");

        Style::ToggleButton("Blink Range", &c.blinkRange);
        Tip("Shows 1200 unit blink range if you have blink dagger");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        Style::ToggleButton("Danger Indicator", &c.dangerIndicator);
        Tip("Arrow indicators pointing to nearby enemies off-screen");

        Style::ToggleButton("Last Hit Helper", &c.lastHitHelper);
        Tip("Highlights creeps killable with one attack");

        ImGui::Unindent(10);
    }
}

// ============================================
// Tab: Auto
// ============================================
static void TabAuto() {
    auto& c = Config::Get();
    Sec("Automation");
    Style::ToggleButton("Auto Accept Match", &c.autoAccept);
    Tip("Automatically accepts when match is found");

    Style::ToggleButton("Auto Mute Enemies", &c.autoMute);
    Tip("Mutes all enemy players at game start");
}

// ============================================
// Tab: Visuals
// ============================================
static void TabVisuals() {
    auto& c = Config::Get();
    Sec("Visual Modifications");

    Style::ToggleButton("Weather Hack", &c.weatherHack);
    if (c.weatherHack) {
        const char* w[] = {
            "Default", "Snow", "Rain", "Moonbeam",
            "Pestilence", "Harvest", "Sirocco", "Spring", "Ash"
        };
        ImGui::Combo("Weather", &c.weatherType, w, IM_ARRAYSIZE(w));
    }

    Style::ToggleButton("Particle Effects", &c.particleHack);
    Tip("Enhanced particle effects for abilities");
}

// ============================================
// Tab: Info (live hero stats)
// ============================================
static void TabInfo() {
    Sec("Local Hero Info");

    if (!SDK::IsInGame()) {
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "Not in game. Join a match to see data.");
        return;
    }

    auto localHero = SDK::GetLocalHero();
    if (!localHero) {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Local hero not found.");
        return;
    }

    if (!localHero->IsAlive()) {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Hero is dead. Respawn time: %.1f",
            localHero->GetRespawnTime());
    }

    auto npc  = reinterpret_cast<C_DOTA_BaseNPC*>(localHero);
    auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(localHero);

    // === Basic Stats ===
    ImGui::BeginChild("##hero_stats", ImVec2(0, 160), true);
    ImGui::Columns(2, "##statcols", false);

    ImGui::Text("Name: %s", npc->GetUnitName());
    ImGui::Text("Hero ID: %d", hero->GetHeroID());
    ImGui::Text("Level: %d", npc->GetLevel());
    ImGui::Text("Primary: %s", hero->GetPrimaryAttributeName());

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "HP: %d / %d (+%.1f/s)",
        npc->GetHealth(), npc->GetMaxHealth(), npc->GetHealthRegen());
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.f, 1.f), "MP: %.0f / %.0f (+%.1f/s)",
        npc->GetMana(), npc->GetMaxMana(), npc->GetManaRegen());

    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "STR: %.1f", hero->GetStrengthTotal());
    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "AGI: %.1f", hero->GetAgilityTotal());
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.f, 1.f), "INT: %.1f", hero->GetIntellectTotal());

    ImGui::Spacing();
    ImGui::Text("Move Speed: %d", npc->GetMoveSpeed());
    ImGui::Text("Attack Range: %d", npc->GetAttackRange());
    ImGui::Text("Day Vision: %d", npc->GetDayVision());
    ImGui::Text("Night Vision: %d", npc->GetNightVision());

    ImGui::Columns(1);
    ImGui::EndChild();

    // === Status Effects ===
    Sec("Status Effects");
    {
        auto Badge = [](const char* label, bool active, ImVec4 activeColor) {
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
                ImGui::SmallButton(label);
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.f));
                ImGui::SmallButton(label);
                ImGui::PopStyleColor(2);
            }
            ImGui::SameLine();
        };

        Badge("STUN",    npc->IsStunned(),      ImVec4(0.9f, 0.8f, 0.1f, 1.f));
        Badge("SILENCE", npc->IsSilenced(),      ImVec4(0.3f, 0.6f, 1.f, 1.f));
        Badge("HEX",     npc->IsHexed(),         ImVec4(0.7f, 0.3f, 1.f, 1.f));
        Badge("ROOT",    npc->IsRooted(),        ImVec4(0.3f, 0.8f, 0.3f, 1.f));
        Badge("INVIS",   npc->IsInvisible(),     ImVec4(0.5f, 0.5f, 1.f, 1.f));
        ImGui::NewLine();

        Badge("BKB",     npc->IsMagicImmune(),   ImVec4(0.9f, 0.7f, 0.1f, 1.f));
        Badge("INVULN",  npc->IsInvulnerable(),  ImVec4(1.f, 1.f, 1.f, 0.8f));
        Badge("DISARM",  npc->IsDisarmed(),      ImVec4(1.f, 0.5f, 0.2f, 1.f));
        Badge("MUTE",    npc->IsMuted(),         ImVec4(0.6f, 0.6f, 0.6f, 1.f));
        Badge("BLIND",   npc->IsBlind(),         ImVec4(0.3f, 0.3f, 0.3f, 1.f));
        ImGui::NewLine();
    }

    // === Abilities ===
    Sec("Abilities");
    {
        std::vector<AbilityInfo> abs;
        SDK::CollectAbilities(npc, abs);

        if (abs.empty()) {
            ImGui::TextDisabled("No abilities found.");
        } else {
            if (ImGui::BeginTable("##abilities", 6,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Name",    ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Lvl",     ImGuiTableColumnFlags_WidthFixed, 30.f);
                ImGui::TableSetupColumn("CD",      ImGuiTableColumnFlags_WidthFixed, 55.f);
                ImGui::TableSetupColumn("Mana",    ImGuiTableColumnFlags_WidthFixed, 45.f);
                ImGui::TableSetupColumn("Charges", ImGuiTableColumnFlags_WidthFixed, 50.f);
                ImGui::TableSetupColumn("State",   ImGuiTableColumnFlags_WidthFixed, 55.f);
                ImGui::TableHeadersRow();

                for (const auto& a : abs) {
                    ImGui::TableNextRow();

                    // Name
                    ImGui::TableNextColumn();
                    if (a.isUltimate)
                        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.3f, 1.f), "%s", a.name.c_str());
                    else
                        ImGui::TextUnformatted(a.name.c_str());

                    // Level
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", a.level);

                    // Cooldown
                    ImGui::TableNextColumn();
                    if (a.isOnCooldown())
                        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%.1f", a.cooldown);
                    else if (a.level > 0)
                        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Ready");
                    else
                        ImGui::TextDisabled("-");

                    // Mana cost
                    ImGui::TableNextColumn();
                    if (a.manaCost > 0)
                        ImGui::Text("%d", a.manaCost);
                    else
                        ImGui::TextDisabled("-");

                    // Charges
                    ImGui::TableNextColumn();
                    if (a.charges > 0)
                        ImGui::Text("%d", a.charges);
                    else
                        ImGui::TextDisabled("-");

                    // State
                    ImGui::TableNextColumn();
                    if (a.inPhase)
                        ImGui::TextColored(ImVec4(1.f, 1.f, 0.3f, 1.f), "CAST");
                    else if (a.isToggled)
                        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.f, 1.f), "ON");
                    else
                        ImGui::TextDisabled("-");
                }

                ImGui::EndTable();
            }
        }
    }

    // === Items ===
    Sec("Inventory");
    {
        std::vector<ItemInfo> items;
        SDK::CollectItems(npc, items);

        if (items.empty()) {
            ImGui::TextDisabled("No items found.");
        } else {
            if (ImGui::BeginTable("##items", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Slot",    ImGuiTableColumnFlags_WidthFixed, 50.f);
                ImGui::TableSetupColumn("Item",    ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Charges", ImGuiTableColumnFlags_WidthFixed, 55.f);
                ImGui::TableSetupColumn("CD",      ImGuiTableColumnFlags_WidthFixed, 55.f);
                ImGui::TableHeadersRow();

                for (const auto& item : items) {
                    ImGui::TableNextRow();

                    // Slot
                    ImGui::TableNextColumn();
                    if (item.slot <= 5)
                        ImGui::Text("Main %d", item.slot);
                    else if (item.slot <= 8)
                        ImGui::Text("Back %d", item.slot - 6);
                    else
                        ImGui::Text("Stash %d", item.slot - 9);

                    // Name
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.name.c_str());

                    // Charges
                    ImGui::TableNextColumn();
                    if (item.charges > 0)
                        ImGui::Text("%d", item.charges);
                    else
                        ImGui::TextDisabled("-");

                    // Cooldown
                    ImGui::TableNextColumn();
                    if (item.isOnCooldown())
                        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%.1f", item.cooldown);
                    else
                        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Ready");
                }

                ImGui::EndTable();
            }
        }
    }

    // === Enemy Heroes ===
    Sec("All Heroes");
    {
        std::vector<HeroData> heroes;
        SDK::CollectHeroes(heroes);

        if (heroes.empty()) {
            ImGui::TextDisabled("No heroes found in entity list.");
        } else {
            ImGui::Text("Found %d heroes:", (int)heroes.size());
            ImGui::Spacing();

            if (ImGui::BeginTable("##heroes", 7,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
            {
                ImGui::TableSetupColumn("Name",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Team",  ImGuiTableColumnFlags_WidthFixed, 40.f);
                ImGui::TableSetupColumn("Lv",    ImGuiTableColumnFlags_WidthFixed, 25.f);
                ImGui::TableSetupColumn("HP",    ImGuiTableColumnFlags_WidthFixed, 70.f);
                ImGui::TableSetupColumn("MP",    ImGuiTableColumnFlags_WidthFixed, 60.f);
                ImGui::TableSetupColumn("Pos",   ImGuiTableColumnFlags_WidthFixed, 120.f);
                ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 80.f);
                ImGui::TableHeadersRow();

                for (const auto& h : heroes) {
                    ImGui::TableNextRow();

                    // Name
                    ImGui::TableNextColumn();
                    ImVec4 nameCol = h.illusion ? ImVec4(1.f, 0.65f, 0.f, 1.f) :
                        (h.team == 2 ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f));
                    ImGui::TextColored(nameCol, "%s", h.name.c_str());

                    // Team
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", h.team == 2 ? "Rad" : (h.team == 3 ? "Dire" : "?"));

                    // Level
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", h.level);

                    // HP
                    ImGui::TableNextColumn();
                    if (h.alive)
                        ImGui::Text("%d/%d", h.health, h.maxHP);
                    else
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "DEAD");

                    // MP
                    ImGui::TableNextColumn();
                    ImGui::Text("%.0f", h.mana);

                    // Position
                    ImGui::TableNextColumn();
                    ImGui::Text("%.0f,%.0f", h.pos.x, h.pos.y);

                    // Flags
                    ImGui::TableNextColumn();
                    std::string flags;
                    if (h.illusion)    flags += "ILL ";
                    if (h.stunned)     flags += "STN ";
                    if (h.silenced)    flags += "SIL ";
                    if (h.hexed)       flags += "HEX ";
                    if (h.invisible)   flags += "INV ";
                    if (h.magicImmune) flags += "BKB ";
                    if (flags.empty()) flags = "-";
                    ImGui::TextUnformatted(flags.c_str());
                }

                ImGui::EndTable();
            }
        }
    }
}

// ============================================
// Tab: Settings + SDK Status
// ============================================
static void TabSettings() {
    auto& c = Config::Get();

    Sec("Menu Settings");
    ImGui::SliderFloat("Opacity", &c.menuAlpha, 0.5f, 1.f, "%.2f");

    const char* accents[] = {"Crimson Red", "Ocean Blue", "Emerald Green", "Royal Purple", "Sunset Orange"};
    if (ImGui::Combo("Accent Color", &c.menuAccent, accents, IM_ARRAYSIZE(accents)))
        Style::ApplyModernDark(c.menuAlpha, c.menuAccent);

    Style::ToggleButton("Stream Proof", &c.streamProof);
    Tip("Hides overlay from OBS (experimental)");

    // === Config ===
    Sec("Config");
    if (ImGui::Button("Save Config", ImVec2(-1, 36)))
        c.Save("dota2cheat.cfg");

    if (ImGui::Button("Load Config", ImVec2(-1, 36))) {
        c.Load("dota2cheat.cfg");
        Style::ApplyModernDark(c.menuAlpha, c.menuAccent);
    }

    if (ImGui::Button("Reset to Default", ImVec2(-1, 36))) {
        c = Config();
        Style::ApplyModernDark(c.menuAlpha, c.menuAccent);
    }

    // === SDK Status ===
    Sec("SDK Status");
    {
        auto StatusLine = [](const char* name, uintptr_t ptr) {
            if (ptr)
                ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "%s: 0x%llX", name, (unsigned long long)ptr);
            else
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s: NOT FOUND", name);
        };

        StatusLine("EntitySystem", SDK::pGameEntitySystem);
        StatusLine("LocalPlayer",  SDK::pLocalPlayer);
        StatusLine("ViewMatrix",   SDK::pViewMatrix);
        StatusLine("GameRules",    SDK::pGameRules);
        StatusLine("Camera",       SDK::pCameraInstance);

        ImGui::Separator();

        if (SDK::IsInGame())
            ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "In Game: YES");
        else
            ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "In Game: NO");

        ImGui::Text("Max Entities: %d", SDK::GetMaxEntities());
        ImGui::Text("Game Time: %.1f", SDK::GetGameTime());

        auto lh = SDK::GetLocalHero();
        if (lh)
            ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Local Hero: %s (HP: %d)",
                lh->GetUnitName(), lh->GetHealth());
        else
            ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "Local Hero: not found");

        auto ctrl = SDK::GetLocalPlayerController();
        if (ctrl)
            ImGui::Text("Player ID: %d | Camera Zoom: %.0f",
                ctrl->GetPlayerID(), ctrl->GetCameraZoom());
    }

    // === Info ===
    Sec("Info");
    ImGui::TextDisabled("Dota 2 Cheat v2.0");
    ImGui::TextDisabled("Build: %s %s", __DATE__, __TIME__);
    ImGui::TextDisabled("INSERT = Toggle Menu");
    ImGui::TextDisabled("END = Unload Cheat");
    ImGui::Spacing();
    ImGui::TextDisabled("FPS: %.0f", ImGui::GetIO().Framerate);
}

// ============================================
// Menu Init / Render / Shutdown
// ============================================

void Menu::Initialize() {
    auto& c = Config::Get();
    Style::ApplyModernDark(c.menuAlpha, c.menuAccent);

    auto& io = ImGui::GetIO();
    ImFontConfig fc;
    fc.OversampleH = 3;
    fc.OversampleV = 2;
    fc.PixelSnapH = true;

    // Try to load a nice system font
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf"
    };

    bool fontLoaded = false;
    for (auto& path : fontPaths) {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            io.Fonts->AddFontFromFileTTF(path, 16.f, &fc);
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        // Use default ImGui font
        io.Fonts->AddFontDefault(&fc);
    }
}

void Menu::Render() {
    if (!G::bMenuOpen) return;

    auto& cfg = Config::Get();
    auto accent = Style::GetAccent(cfg.menuAccent);

    // Window setup
    ImGui::SetNextWindowSize(ImVec2(700, 560), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 400), ImVec2(1200, 900));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##MainWindow", &G::bMenuOpen, windowFlags)) {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::PopStyleVar();

    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();
    auto* dl = ImGui::GetWindowDrawList();

    // === Animated background ===
    DrawP(wp, ws);

    // === Header ===
    float headerH = 52.f;
    ImU32 headerBg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.06f, 0.08f, 1.f));
    ImU32 accentCol = ImGui::ColorConvertFloat4ToU32(accent.primary);
    ImU32 accentFade = ImGui::ColorConvertFloat4ToU32(
        ImVec4(accent.primary.x, accent.primary.y, accent.primary.z, 0.f));

    dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + headerH), headerBg, 12.f, ImDrawFlags_RoundCornersTop);

    // Gradient accent line
    dl->AddRectFilledMultiColor(
        ImVec2(wp.x, wp.y + headerH - 3),
        ImVec2(wp.x + ws.x, wp.y + headerH),
        accentCol, accentFade, accentFade, accentCol
    );

    // Title
    dl->AddText(ImVec2(wp.x + 20, wp.y + 16), accentCol, "DOTA 2 CHEAT");

    // Version + In-game status
    {
        const char* status = SDK::IsInGame() ? "IN GAME" : "LOBBY";
        ImU32 statusCol = SDK::IsInGame()
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 1.f, 0.4f, 1.f))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.55f, 1.f));

        auto statusSize = ImGui::CalcTextSize(status);
        dl->AddText(ImVec2(wp.x + ws.x - statusSize.x - 20, wp.y + 10), statusCol, status);
        dl->AddText(ImVec2(wp.x + ws.x - 50, wp.y + 28),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.4f, 0.45f, 1.f)), "v2.0");
    }

    ImGui::SetCursorPosY(headerH);

    // === Sidebar ===
    float sidebarW = 145.f;
    ImGui::SetCursorPos(ImVec2(0, headerH));
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, ws.y - headerH), false, ImGuiWindowFlags_NoScrollbar);

    auto sidebarPos = ImGui::GetWindowPos();
    ImU32 sidebarBg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.06f, 0.08f, 0.95f));
    dl->AddRectFilled(sidebarPos,
        ImVec2(sidebarPos.x + sidebarW, sidebarPos.y + ws.y - headerH), sidebarBg);

    ImGui::Spacing();
    ImGui::Spacing();

    for (int i = 0; i < IM_ARRAYSIZE(TABS); i++) {
        ImGui::PushID(i);
        bool selected = (curTab == i);

        char label[64];
        snprintf(label, sizeof(label), "  %s  %s", ICONS[i], TABS[i]);

        auto btnPos = ImGui::GetCursorScreenPos();
        float btnH = 36.f;
        float btnW = sidebarW - 16.f;
        ImGui::SetCursorPosX(8.f);

        if (selected) {
            // Active indicator line
            dl->AddRectFilled(
                ImVec2(btnPos.x, btnPos.y),
                ImVec2(btnPos.x + 3, btnPos.y + btnH),
                accentCol, 2.f
            );
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImVec4(accent.primary.x, accent.primary.y, accent.primary.z, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(accent.primary.x, accent.primary.y, accent.primary.z, 0.25f));
            ImGui::PushStyleColor(ImGuiCol_Text, accent.primary);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.f));
        }

        if (ImGui::Button(label, ImVec2(btnW, btnH)))
            curTab = i;

        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }

    // Bottom FPS
    auto sidebarSize = ImGui::GetWindowSize();
    ImGui::SetCursorPosY(sidebarSize.y - 25);
    ImGui::SetCursorPosX(12.f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.40f, 1.f));
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();

    ImGui::EndChild();

    // === Content area ===
    ImGui::SetCursorPos(ImVec2(sidebarW, headerH));
    ImGui::BeginChild("##content", ImVec2(ws.x - sidebarW, ws.y - headerH), false,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::SetCursorPos(ImVec2(20, 16));

    switch (curTab) {
        case 0: TabCamera();    break;
        case 1: TabESP();       break;
        case 2: TabAwareness(); break;
        case 3: TabAuto();      break;
        case 4: TabVisuals();   break;
        case 5: TabInfo();      break;
        case 6: TabSettings();  break;
    }

    ImGui::EndChild();

    ImGui::End();
}

void Menu::Shutdown() {
    // Nothing to clean up
}
