#include "esp.h"
#include "../sdk/interfaces.h"
#include "../config.h"
#include "../render/renderer.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace ESP {
    static ImU32 HPColor(float p) {
        if (p > 0.6f) return IM_COL32((int)Math::Lerp(230,80,(p-0.6f)/0.4f), (int)Math::Lerp(200,220,(p-0.6f)/0.4f), (int)Math::Lerp(50,80,(p-0.6f)/0.4f), 255);
        if (p > 0.3f) return IM_COL32(230, (int)Math::Lerp(100,200,(p-0.3f)/0.3f), 50, 255);
        return IM_COL32(230, 60, 60, 255);
    }

    static std::string CleanName(const std::string& raw) {
        std::string n = raw;
        const char* pfx[] = {"npc_dota_hero_","C_DOTA_Unit_Hero_","CDOTA_Unit_Hero_"};
        for (auto p : pfx) { auto pos = n.find(p); if (pos != std::string::npos) { n = n.substr(pos + strlen(p)); break; } }
        for (size_t i = 0; i < n.size(); i++) { if (n[i]=='_') n[i]=' '; if (i==0||(i>0&&n[i-1]==' ')) n[i]=toupper(n[i]); }
        return n;
    }

    static void DrawHero(const HeroData& h, const Matrix4x4& vm) {
        auto& c = Config::Get();
        Vector2 sp;
        if (!Math::WorldToScreen(h.pos, sp, vm, G::screenWidth, G::screenHeight)) return;
        float x = sp.x, y = sp.y;

        if (c.espIllusions && h.illusion) {
            static float pt = 0.f; pt += ImGui::GetIO().DeltaTime;
            float pa = (sinf(pt*5.f)+1.f)*0.5f;
            Renderer::DrawText(x, y-55, IM_COL32(255,165,0,(int)(pa*180+75)), "ILLUSION", c.espFontSize, true);
            Renderer::DrawCircle(x, y, 28+pa*5, IM_COL32(255,165,0,(int)(pa*150)), 32, 2);
        }

        if (h.stunned) Renderer::DrawText(x, y-42, IM_COL32(255,255,50,255), "[STUN]", c.espFontSize, true);
        else if (h.hexed) Renderer::DrawText(x, y-42, IM_COL32(200,100,255,255), "[HEX]", c.espFontSize, true);
        else if (h.silenced) Renderer::DrawText(x, y-42, IM_COL32(80,180,255,255), "[SIL]", c.espFontSize, true);

        if (h.magicImmune) Renderer::DrawCircle(x, y, 30, IM_COL32(200,170,50,150), 32, 2.5f);

        if (c.espNames) {
            char nl[128]; snprintf(nl, 128, "%s [%d] %s", CleanName(h.name).c_str(), h.level, h.primaryAttr.c_str());
            ImU32 nc = h.illusion ? IM_COL32(255,165,0,255) : (h.team==2 ? IM_COL32(100,220,100,255) : IM_COL32(220,100,100,255));
            Renderer::DrawText(x, y-30, nc, nl, c.espFontSize, true);
        }

        if (c.espHealth && h.maxHP > 0) {
            float hp = (float)h.health / h.maxHP;
            Renderer::DrawProgressBar(x-32, y-16, 64, 7, hp, HPColor(hp), IM_COL32(20,20,25,200));
            char ht[32]; snprintf(ht, 32, "%d/%d", h.health, h.maxHP);
            Renderer::DrawText(x, y-17, IM_COL32(255,255,255,200), ht, 10, true);
        }

        if (c.espMana && h.maxMana > 0) {
            Renderer::DrawProgressBar(x-32, y-7, 64, 4, h.mana/h.maxMana, IM_COL32(70,130,230,255), IM_COL32(20,20,40,200));
        }

        if (c.espSpellTracker && !h.abilities.empty()) {
            std::vector<const AbilityInfo*> da;
            for (auto& a : h.abilities) { if (a.isHidden && a.level==0) continue; if (!a.isActivated && a.level==0) continue; da.push_back(&a); if (da.size()>=7) break; }
            if (!da.empty()) {
                float bs=18, sp2=3, tw=da.size()*(bs+sp2)-sp2, sx=x-tw*0.5f, sy=y+6;
                for (size_t i=0; i<da.size(); i++) {
                    float bx = sx + i*(bs+sp2);
                    auto& a = *da[i];
                    if (a.level==0) { Renderer::DrawFilledRect(bx,sy,bs,bs,IM_COL32(35,35,40,200)); }
                    else if (a.isOnCooldown()) {
                        Renderer::DrawFilledRect(bx,sy,bs,bs,IM_COL32(35,35,40,220));
                        float cp=1.f-a.getCooldownPercent(), fh=bs*cp;
                        Renderer::DrawFilledRect(bx,sy+bs-fh,bs,fh,IM_COL32(120,30,30,220));
                        char ct[8]; snprintf(ct,8,a.cooldown>=10?"%.0f":"%.1f",a.cooldown);
                        Renderer::DrawText(bx+bs*0.5f,sy+3,IM_COL32(255,120,120,255),ct,9,true);
                    } else { Renderer::DrawFilledRect(bx,sy,bs,bs,IM_COL32(30,120,60,220)); }
                    Renderer::DrawRect(bx,sy,bs,bs, a.isUltimate ? IM_COL32(220,180,50,255) : IM_COL32(70,70,80,255), 1);
                }
            }
        }

        if (c.espItems && !h.items.empty()) {
            float iw=22, ih=14, isp=2; int dc=0;
            for (auto& it : h.items) if (it.slot>=0&&it.slot<=5) dc++;
            if (dc > 0) {
                float tw=dc*(iw+isp)-isp, sx=x-tw*0.5f, sy=y+35;
                int dr=0;
                for (auto& it : h.items) {
                    if (it.slot<0||it.slot>5) continue;
                    float ix=sx+dr*(iw+isp);
                    Renderer::DrawFilledRect(ix,sy,iw,ih, it.isOnCooldown() ? IM_COL32(60,30,30,200) : IM_COL32(40,50,70,200));
                    Renderer::DrawRect(ix,sy,iw,ih,IM_COL32(80,80,100,200),1);
                    std::string sn=it.name; auto p=sn.find("item_"); if(p!=std::string::npos)sn=sn.substr(p+5,3); else if(sn.size()>3)sn=sn.substr(0,3);
                    Renderer::DrawText(ix+iw*0.5f,sy+1,IM_COL32(200,200,210,220),sn.c_str(),8,true);
                    dr++;
                }
            }
        }
    }

    void OnFrame() {
        auto& c = Config::Get();
        if (!c.espEnabled) return;
        auto vm = SDK::GetViewMatrix();
        auto lh = SDK::GetLocalHero();
        if (!lh) return;
        uint8_t lt = lh->GetTeam();
        std::vector<HeroData> heroes;
        SDK::CollectHeroes(heroes);
        for (auto& h : heroes) { if (h.team==lt && !h.illusion) continue; if (!h.alive) continue; DrawHero(h, vm); }
    }
}
