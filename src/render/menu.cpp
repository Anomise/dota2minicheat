#include "menu.h"
#include "style.h"
#include "../globals.h"
#include "../config.h"
#include "../sdk/interfaces.h"
#include "../utils/logger.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* TABS[]  = {"CAMERA","ESP","AWARENESS","AUTO","VISUALS","INFO","LOG","SETTINGS"};
static const char* ICONS[] = {"[C]","[E]","[A]","[M]","[V]","[I]","[L]","[S]"};
static int curTab = 0;

// Particles
struct Particle { float x,y,vx,vy,a,s; };
static std::vector<Particle> particles;
static bool pInit = false;

static void InitP() {
    particles.resize(40);
    for(auto&p:particles){
        p.x=(float)(rand()%600);p.y=(float)(rand()%500);
        p.vx=((rand()%100)/100.f-0.5f)*0.3f;p.vy=((rand()%100)/100.f-0.5f)*0.3f;
        p.a=(rand()%60+20)/255.f;p.s=(float)(rand()%3+1);
    }
    pInit=true;
}

static void DrawP(ImVec2 o,ImVec2 sz) {
    if(!pInit)InitP();
    auto*dl=ImGui::GetWindowDrawList();
    for(auto&p:particles){
        p.x+=p.vx;p.y+=p.vy;
        if(p.x<0||p.x>sz.x)p.vx=-p.vx;
        if(p.y<0||p.y>sz.y)p.vy=-p.vy;
        dl->AddCircleFilled({o.x+p.x,o.y+p.y},p.s,
            ImGui::ColorConvertFloat4ToU32({1,1,1,p.a}));
    }
}

static void Sec(const char*l){
    auto a=Style::GetAccent(Config::Get().menuAccent);
    ImGui::Spacing();ImGui::PushStyleColor(ImGuiCol_Text,a.primary);
    ImGui::TextUnformatted(l);ImGui::PopStyleColor();
    Style::DrawGlowLine(a.primary,2);
}

static void Tip(const char*d){
    ImGui::SameLine();ImGui::TextDisabled("(?)");
    if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::PushTextWrapPos(300);
        ImGui::TextUnformatted(d);ImGui::PopTextWrapPos();ImGui::EndTooltip();}
}

static void TabCamera(){
    auto&c=Config::Get();Sec("Camera Hack");
    Style::ToggleButton("Enable Camera Hack",&c.camerHack);Tip("Unlocks camera distance");
    if(c.camerHack){ImGui::Indent(10);
        ImGui::SliderFloat("Distance",&c.cameraDistance,800,5000,"%.0f");
        ImGui::Text("Presets:");ImGui::SameLine();
        if(ImGui::SmallButton("Default"))c.cameraDistance=1200;ImGui::SameLine();
        if(ImGui::SmallButton("Medium"))c.cameraDistance=1600;ImGui::SameLine();
        if(ImGui::SmallButton("Far"))c.cameraDistance=2000;ImGui::SameLine();
        if(ImGui::SmallButton("Ultra"))c.cameraDistance=3000;
        ImGui::SliderFloat("FOV Offset",&c.cameraFOV,-30,30,"%.1f");
        Style::ToggleButton("Remove Fog",&c.cameraFog);
        ImGui::Unindent(10);}
}

static void TabESP(){
    auto&c=Config::Get();Sec("Enemy ESP");
    Style::ToggleButton("Enable ESP",&c.espEnabled);
    if(c.espEnabled){ImGui::Indent(10);
        ImGui::Columns(2,"##ec",false);
        Style::ToggleButton("Health Bars",&c.espHealth);
        Style::ToggleButton("Mana Bars",&c.espMana);
        Style::ToggleButton("Hero Names",&c.espNames);
        ImGui::NextColumn();
        Style::ToggleButton("Spell Tracker",&c.espSpellTracker);
        Style::ToggleButton("Items",&c.espItems);
        Style::ToggleButton("Illusion Detect",&c.espIllusions);
        ImGui::Columns(1);ImGui::Separator();
        ImGui::SliderFloat("Font Size",&c.espFontSize,10,24,"%.0f");
        ImGui::Unindent(10);}
}

static void TabAwareness(){
    auto&c=Config::Get();Sec("Map Awareness");
    Style::ToggleButton("Enable Awareness",&c.awarenessEnabled);
    if(c.awarenessEnabled){ImGui::Indent(10);
        Style::ToggleButton("Tower Range",&c.towerRange);
        if(c.towerRange)ImGui::ColorEdit4("Tower Color",c.towerRangeColor,ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_AlphaBar);
        Style::ToggleButton("XP Range",&c.expRange);
        if(c.expRange)ImGui::ColorEdit4("XP Color",c.expRangeColor,ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_AlphaBar);
        Style::ToggleButton("Attack Range",&c.attackRange);
        Style::ToggleButton("Blink Range",&c.blinkRange);
        ImGui::Separator();
        Style::ToggleButton("Danger Indicator",&c.dangerIndicator);
        Style::ToggleButton("Last Hit Helper",&c.lastHitHelper);
        ImGui::Unindent(10);}
}

static void TabAuto(){
    auto&c=Config::Get();Sec("Automation");
    Style::ToggleButton("Auto Accept",&c.autoAccept);
    Style::ToggleButton("Auto Mute",&c.autoMute);
}

static void TabVisuals(){
    auto&c=Config::Get();Sec("Visuals");
    Style::ToggleButton("Weather Hack",&c.weatherHack);
    if(c.weatherHack){const char*w[]={"Default","Snow","Rain","Moonbeam","Pestilence","Harvest","Sirocco","Spring","Ash"};
        ImGui::Combo("Weather",&c.weatherType,w,IM_ARRAYSIZE(w));}
}

static void TabInfo(){
    Sec("Local Hero Info");
    if(!SDK::IsInGame()){ImGui::TextColored({1,.6f,.2f,1},"Not in game.");return;}
    auto lh=SDK::GetLocalHero();
    if(!lh){ImGui::TextColored({1,.4f,.4f,1},"Hero not found.");return;}
    auto npc=reinterpret_cast<C_DOTA_BaseNPC*>(lh);
    auto hero=reinterpret_cast<C_DOTA_BaseNPC_Hero*>(lh);

    ImGui::BeginChild("##hs",{0,160},true);
    ImGui::Columns(2,"##sc",false);
    ImGui::Text("Name: %s",npc->GetUnitName());
    ImGui::Text("ID:%d Lv:%d",hero->GetHeroID(),npc->GetLevel());
    ImGui::TextColored({.4f,1,.4f,1},"HP:%d/%d (+%.1f)",npc->GetHealth(),npc->GetMaxHealth(),npc->GetHealthRegen());
    ImGui::TextColored({.4f,.6f,1,1},"MP:%.0f/%.0f (+%.1f)",npc->GetMana(),npc->GetMaxMana(),npc->GetManaRegen());
    ImGui::NextColumn();
    ImGui::TextColored({1,.4f,.4f,1},"STR:%.1f",hero->GetStrengthTotal());
    ImGui::TextColored({.4f,1,.4f,1},"AGI:%.1f",hero->GetAgilityTotal());
    ImGui::TextColored({.4f,.7f,1,1},"INT:%.1f",hero->GetIntellectTotal());
    ImGui::Text("MS:%d AR:%d",npc->GetMoveSpeed(),npc->GetAttackRange());
    ImGui::Columns(1);ImGui::EndChild();

    Sec("Abilities");
    std::vector<AbilityInfo> abs;SDK::CollectAbilities(npc,abs);
    if(!abs.empty()&&ImGui::BeginTable("##ab",5,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg)){
        ImGui::TableSetupColumn("Name");ImGui::TableSetupColumn("Lv",0,30);
        ImGui::TableSetupColumn("CD",0,50);ImGui::TableSetupColumn("Mana",0,40);
        ImGui::TableSetupColumn("Chrg",0,40);ImGui::TableHeadersRow();
        for(auto&a:abs){ImGui::TableNextRow();ImGui::TableNextColumn();
            if(a.isUltimate)ImGui::TextColored({1,.85f,.3f,1},"%s",a.name.c_str());
            else ImGui::TextUnformatted(a.name.c_str());
            ImGui::TableNextColumn();ImGui::Text("%d",a.level);
            ImGui::TableNextColumn();
            if(a.isOnCooldown())ImGui::TextColored({1,.4f,.4f,1},"%.1f",a.cooldown);
            else if(a.level>0)ImGui::TextColored({.4f,1,.4f,1},"Rdy");else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();ImGui::Text("%d",a.manaCost);
            ImGui::TableNextColumn();if(a.charges>0)ImGui::Text("%d",a.charges);else ImGui::TextDisabled("-");}
        ImGui::EndTable();}

    Sec("All Heroes");
    std::vector<HeroData> heroes;SDK::CollectHeroes(heroes);
    if(!heroes.empty()&&ImGui::BeginTable("##heroes",6,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY,{0,180})){
        ImGui::TableSetupColumn("Name");ImGui::TableSetupColumn("Team",0,40);
        ImGui::TableSetupColumn("Lv",0,25);ImGui::TableSetupColumn("HP",0,70);
        ImGui::TableSetupColumn("Pos",0,110);ImGui::TableSetupColumn("Flags",0,70);
        ImGui::TableHeadersRow();
        for(auto&h:heroes){ImGui::TableNextRow();ImGui::TableNextColumn();
            ImVec4 nc=h.illusion?ImVec4(1,.65f,0,1):(h.team==2?ImVec4(.4f,1,.4f,1):ImVec4(1,.4f,.4f,1));
            ImGui::TextColored(nc,"%s",h.name.c_str());
            ImGui::TableNextColumn();ImGui::Text("%s",h.team==2?"Rad":(h.team==3?"Dire":"?"));
            ImGui::TableNextColumn();ImGui::Text("%d",h.level);
            ImGui::TableNextColumn();
            if(h.alive)ImGui::Text("%d/%d",h.health,h.maxHP);else ImGui::TextDisabled("DEAD");
            ImGui::TableNextColumn();ImGui::Text("%.0f,%.0f",h.pos.x,h.pos.y);
            ImGui::TableNextColumn();std::string fl;
            if(h.illusion)fl+="ILL ";if(h.stunned)fl+="STN ";if(h.invisible)fl+="INV ";
            ImGui::TextUnformatted(fl.empty()?"-":fl.c_str());}
        ImGui::EndTable();}
}

// ============================================
// NEW: LOG TAB
// ============================================
static void TabLog() {
    Sec("Debug Log");

    auto& logger = Logger::Get();
    auto entries = logger.GetEntries();

    // Controls
    ImGui::Text("Entries: %d", (int)entries.size());
    ImGui::SameLine();
    if (ImGui::Button("Clear Log")) logger.Clear();
    ImGui::SameLine();
    if (ImGui::Button("Copy All")) {
        std::string all;
        for (auto& e : entries) {
            all += e.message + "\n";
        }
        ImGui::SetClipboardText(all.c_str());
    }

    // Filter
    static int filterLevel = -1; // -1 = all
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    const char* filters[] = {"All", "Info", "Warning", "Error", "Success"};
    ImGui::Combo("##filter", &filterLevel, filters, IM_ARRAYSIZE(filters));
    filterLevel -= 1; // Adjust: -1=all, 0=info, 1=warn, 2=error, 3=success

    static bool autoScroll = true;
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    // Log entries
    ImGui::BeginChild("##logscroll", ImVec2(0, 0), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);

    for (auto& entry : entries) {
        if (filterLevel >= 0 && entry.level != filterLevel) continue;

        ImVec4 color;
        const char* prefix;
        switch (entry.level) {
            case 0: color = ImVec4(0.7f, 0.7f, 0.75f, 1.f); prefix = "[INFO]   "; break;
            case 1: color = ImVec4(1.f, 0.85f, 0.3f, 1.f);  prefix = "[WARN]   "; break;
            case 2: color = ImVec4(1.f, 0.4f, 0.4f, 1.f);   prefix = "[ERROR]  "; break;
            case 3: color = ImVec4(0.4f, 1.f, 0.4f, 1.f);   prefix = "[OK]     "; break;
            default: color = ImVec4(0.6f, 0.6f, 0.6f, 1.f);  prefix = "[?]      "; break;
        }

        // Timestamp
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.45f, 1.f), "[%7.2f]", entry.timestamp);
        ImGui::SameLine();

        // Message
        ImGui::TextColored(color, "%s%s", prefix, entry.message.c_str());
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

static void TabSettings(){
    auto&c=Config::Get();Sec("Menu Settings");
    ImGui::SliderFloat("Opacity",&c.menuAlpha,0.5f,1,"%.2f");
    const char*ac[]={"Crimson","Ocean","Emerald","Purple","Orange"};
    if(ImGui::Combo("Accent",&c.menuAccent,ac,IM_ARRAYSIZE(ac)))
        Style::ApplyModernDark(c.menuAlpha,c.menuAccent);
    Style::ToggleButton("Stream Proof",&c.streamProof);

    Sec("Config");
    if(ImGui::Button("Save",{-1,36}))c.Save("dota2cheat.cfg");
    if(ImGui::Button("Load",{-1,36})){c.Load("dota2cheat.cfg");Style::ApplyModernDark(c.menuAlpha,c.menuAccent);}
    if(ImGui::Button("Reset",{-1,36})){c=Config();Style::ApplyModernDark(c.menuAlpha,c.menuAccent);}

    Sec("SDK Status");
    auto SL=[](const char*n,uintptr_t p){
        if(p)ImGui::TextColored({.4f,1,.4f,1},"%-20s 0x%llX",n,(unsigned long long)p);
        else ImGui::TextColored({1,.4f,.4f,1},"%-20s NOT FOUND",n);};
    SL("EntitySystem",SDK::pGameEntitySystem);
    SL("LocalPlayer",SDK::pLocalPlayer);
    SL("ViewMatrix",SDK::pViewMatrix);
    SL("GameRules",SDK::pGameRules);
    SL("Camera",SDK::pCameraInstance);
    ImGui::Separator();
    ImGui::Text("In Game: %s",SDK::IsInGame()?"YES":"NO");
    ImGui::Text("Max Entities: %d",SDK::GetMaxEntities());
    ImGui::Text("Game Time: %.1f",SDK::GetGameTime());
    auto lh=SDK::GetLocalHero();
    if(lh)ImGui::TextColored({.4f,1,.4f,1},"Hero: %s HP:%d",lh->GetUnitName(),lh->GetHealth());
    else ImGui::TextColored({1,.6f,.2f,1},"Hero: not found");

    // Live ViewMatrix debug
    Sec("ViewMatrix Debug");
    auto vm = SDK::GetViewMatrix();
    bool vmValid = false;
    for(int i=0;i<4&&!vmValid;i++)for(int j=0;j<4&&!vmValid;j++)if(vm.m[i][j]!=0.f)vmValid=true;
    if(vmValid){
        ImGui::TextColored({.4f,1,.4f,1},"ViewMatrix: VALID");
        ImGui::Text("[%.2f %.2f %.2f %.2f]",vm.m[0][0],vm.m[0][1],vm.m[0][2],vm.m[0][3]);
        ImGui::Text("[%.2f %.2f %.2f %.2f]",vm.m[1][0],vm.m[1][1],vm.m[1][2],vm.m[1][3]);
        ImGui::Text("[%.2f %.2f %.2f %.2f]",vm.m[2][0],vm.m[2][1],vm.m[2][2],vm.m[2][3]);
        ImGui::Text("[%.2f %.2f %.2f %.2f]",vm.m[3][0],vm.m[3][1],vm.m[3][2],vm.m[3][3]);
    } else {
        ImGui::TextColored({1,.4f,.4f,1},"ViewMatrix: INVALID (all zeros)");
    }

    Sec("Info");
    ImGui::TextDisabled("Dota2 Cheat v2.0 | %s %s",__DATE__,__TIME__);
    ImGui::TextDisabled("INSERT=Menu END=Unload FPS:%.0f",ImGui::GetIO().Framerate);
    ImGui::TextDisabled("Log entries: %d", Logger::Get().GetCount());
}

void Menu::Initialize(){
    auto&c=Config::Get();Style::ApplyModernDark(c.menuAlpha,c.menuAccent);
    auto&io=ImGui::GetIO();ImFontConfig fc;fc.OversampleH=3;fc.OversampleV=2;fc.PixelSnapH=true;
    const char*fonts[]={"C:\\Windows\\Fonts\\segoeui.ttf","C:\\Windows\\Fonts\\arial.ttf","C:\\Windows\\Fonts\\tahoma.ttf"};
    bool loaded=false;
    for(auto f:fonts){if(GetFileAttributesA(f)!=INVALID_FILE_ATTRIBUTES){io.Fonts->AddFontFromFileTTF(f,16,&fc);loaded=true;break;}}
    if(!loaded)io.Fonts->AddFontDefault(&fc);
}

void Menu::Render(){
    if(!G::bMenuOpen)return;
    auto&cfg=Config::Get();auto accent=Style::GetAccent(cfg.menuAccent);

    ImGui::SetNextWindowSize({720,580},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({100,100},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({500,400},{1400,1000});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    if(!ImGui::Begin("##MW",&G::bMenuOpen,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        ImGui::End();ImGui::PopStyleVar();return;}
    ImGui::PopStyleVar();

    auto wp=ImGui::GetWindowPos(),ws=ImGui::GetWindowSize();
    auto*dl=ImGui::GetWindowDrawList();
    DrawP(wp,ws);

    float hH=52;
    ImU32 acCol=ImGui::ColorConvertFloat4ToU32(accent.primary);
    ImU32 acFade=ImGui::ColorConvertFloat4ToU32({accent.primary.x,accent.primary.y,accent.primary.z,0});
    dl->AddRectFilled(wp,{wp.x+ws.x,wp.y+hH},ImGui::ColorConvertFloat4ToU32({0.06f,0.06f,0.08f,1}),12,ImDrawFlags_RoundCornersTop);
    dl->AddRectFilledMultiColor({wp.x,wp.y+hH-3},{wp.x+ws.x,wp.y+hH},acCol,acFade,acFade,acCol);
    dl->AddText({wp.x+20,wp.y+16},acCol,"DOTA 2 CHEAT");

    // Status indicator
    const char*status=SDK::IsInGame()?"IN GAME":"LOBBY";
    ImU32 stCol=SDK::IsInGame()?ImGui::ColorConvertFloat4ToU32({.4f,1,.4f,1}):ImGui::ColorConvertFloat4ToU32({.5f,.5f,.55f,1});
    auto stSz=ImGui::CalcTextSize(status);
    dl->AddText({wp.x+ws.x-stSz.x-20,wp.y+10},stCol,status);
    dl->AddText({wp.x+ws.x-50,wp.y+28},ImGui::ColorConvertFloat4ToU32({.4f,.4f,.45f,1}),"v2.0");

    // Log count badge
    int logCount = Logger::Get().GetCount();
    if (logCount > 0) {
        char badge[16]; snprintf(badge, 16, "%d", logCount);
        auto badgeSz = ImGui::CalcTextSize(badge);
        float bx = wp.x + ws.x - stSz.x - badgeSz.x - 45;
        dl->AddText({bx, wp.y + 28}, ImGui::ColorConvertFloat4ToU32({.5f,.5f,.55f,1}), badge);
        dl->AddText({bx + badgeSz.x + 2, wp.y + 28}, ImGui::ColorConvertFloat4ToU32({.4f,.4f,.45f,1}), "logs");
    }

    ImGui::SetCursorPosY(hH);

    float sbW=150;
    ImGui::SetCursorPos({0,hH});
    ImGui::BeginChild("##sb",{sbW,ws.y-hH},false,ImGuiWindowFlags_NoScrollbar);
    auto sp=ImGui::GetWindowPos();
    dl->AddRectFilled(sp,{sp.x+sbW,sp.y+ws.y-hH},ImGui::ColorConvertFloat4ToU32({0.06f,0.06f,0.08f,0.95f}));
    ImGui::Spacing();ImGui::Spacing();

    for(int i=0;i<IM_ARRAYSIZE(TABS);i++){
        ImGui::PushID(i);bool sel=(curTab==i);
        char lbl[64];snprintf(lbl,64,"  %s  %s",ICONS[i],TABS[i]);
        auto bp=ImGui::GetCursorScreenPos();float bH=34,bW=sbW-16;
        ImGui::SetCursorPosX(8);
        if(sel){dl->AddRectFilled({bp.x,bp.y},{bp.x+3,bp.y+bH},acCol,2);
            ImGui::PushStyleColor(ImGuiCol_Button,{accent.primary.x,accent.primary.y,accent.primary.z,0.15f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{accent.primary.x,accent.primary.y,accent.primary.z,0.25f});
            ImGui::PushStyleColor(ImGuiCol_Text,accent.primary);
        }else{ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{1,1,1,0.05f});
            ImGui::PushStyleColor(ImGuiCol_Text,{0.6f,0.6f,0.65f,1});}
        if(ImGui::Button(lbl,{bW,bH}))curTab=i;
        ImGui::PopStyleColor(3);ImGui::PopID();
    }

    auto sbSz=ImGui::GetWindowSize();
    ImGui::SetCursorPosY(sbSz.y-25);ImGui::SetCursorPosX(12);
    ImGui::PushStyleColor(ImGuiCol_Text,{.35f,.35f,.4f,1});
    ImGui::Text("FPS:%.0f",ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::SetCursorPos({sbW,hH});
    ImGui::BeginChild("##ct",{ws.x-sbW,ws.y-hH},false,ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::SetCursorPos({20,16});
    switch(curTab){
        case 0:TabCamera();break;case 1:TabESP();break;case 2:TabAwareness();break;
        case 3:TabAuto();break;case 4:TabVisuals();break;case 5:TabInfo();break;
        case 6:TabLog();break;case 7:TabSettings();break;
    }
    ImGui::EndChild();
    ImGui::End();
}

void Menu::Shutdown(){}
