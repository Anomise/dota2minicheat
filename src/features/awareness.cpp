#include "awareness.h"
#include "../sdk/interfaces.h"
#include "../config.h"
#include "../render/renderer.h"
#include <cstring>
#include <cmath>

namespace Awareness {
    static void DrawWorldCircle(const Vector3& c, float r, ImU32 col, const Matrix4x4& vm, int seg=64, float th=2.f) {
        Vector2 prev; bool pv=false;
        for (int i=0; i<=seg; i++) {
            float a=(float)i/seg*6.2831853f;
            Vector3 wp(c.x+cosf(a)*r, c.y+sinf(a)*r, c.z);
            Vector2 sp; bool v=Math::WorldToScreen(wp, sp, vm, G::screenWidth, G::screenHeight);
            if (v && pv) Renderer::DrawLine(prev.x, prev.y, sp.x, sp.y, col, th);
            prev=sp; pv=v;
        }
    }

    void OnFrame() {
        auto& cfg = Config::Get();
        if (!cfg.awarenessEnabled) return;
        auto lh = SDK::GetLocalHero();
        if (!lh || !lh->IsAlive()) return;
        auto vm = SDK::GetViewMatrix();
        auto hp = lh->GetPosition();
        auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(lh);

        if (cfg.attackRange) { int r=npc->GetAttackRange(); if(r>0) DrawWorldCircle(hp,(float)r,IM_COL32(255,255,255,60),vm,64,1.5f); }

        if (cfg.blinkRange) {
            bool has=false;
            for (int s=0;s<6;s++) { auto h=npc->GetItemHandle(s); if(h==0xFFFFFFFF||h==0)continue;
                auto e=SDK::GetEntityFromHandle(h); if(!e)continue; auto n=e->GetClassName();
                if(n&&strstr(n,"blink")){has=true;break;} }
            if (has) DrawWorldCircle(hp, 1200.f, IM_COL32(100,180,255,80), vm, 48, 1.5f);
        }

        if (cfg.expRange) {
            ImU32 xc=ImGui::ColorConvertFloat4ToU32(ImVec4(cfg.expRangeColor[0],cfg.expRangeColor[1],cfg.expRangeColor[2],cfg.expRangeColor[3]));
            DrawWorldCircle(hp, 1500.f, xc, vm, 48, 1.f);
        }

        if (cfg.towerRange) {
            ImU32 tc=ImGui::ColorConvertFloat4ToU32(ImVec4(cfg.towerRangeColor[0],cfg.towerRangeColor[1],cfg.towerRangeColor[2],cfg.towerRangeColor[3]));
            int mx=SDK::GetMaxEntities(); if(mx<=0||mx>20000)mx=2048;
            uint8_t lt=lh->GetTeam();
            for (int i=0;i<mx;i++) {
                auto e=SDK::GetEntityByIndex(i); if(!e||!e->IsAlive())continue;
                auto n=e->GetClassName(); if(!n||!strstr(n,"Tower"))continue;
                if(e->GetTeam()==lt)continue;
                auto tp=e->GetPosition(); float d=hp.Distance2D(tp);
                if (d<3500.f) {
                    DrawWorldCircle(tp, 700.f, tc, vm, 48, 2.5f);
                    if (d<700.f) {
                        static float wp2=0; wp2+=ImGui::GetIO().DeltaTime;
                        int al=(int)((sinf(wp2*6.f)+1.f)*0.5f*200+55);
                        Renderer::DrawRect(0,0,G::screenWidth,G::screenHeight,IM_COL32(255,30,30,al/4),4.f);
                        Vector2 hs; if(Math::WorldToScreen(hp,hs,vm,G::screenWidth,G::screenHeight))
                            Renderer::DrawText(hs.x,hs.y-70,IM_COL32(255,50,50,al),"! TOWER RANGE !",18,true);
                    }
                }
            }
        }

        if (cfg.dangerIndicator) {
            std::vector<HeroData> heroes; SDK::CollectHeroes(heroes);
            uint8_t lt=lh->GetTeam();
            for (auto& en : heroes) {
                if(en.team==lt||!en.alive) continue;
                float d=hp.Distance2D(en.pos); if(d>3000||d<50) continue;
                Vector2 sp; if(!Math::WorldToScreen(en.pos,sp,vm,G::screenWidth,G::screenHeight)) {
                    Vector3 dir=en.pos-hp; float a=atan2f(dir.y,dir.x);
                    float ex=G::screenWidth*0.5f+cosf(a)*G::screenWidth*0.45f;
                    float ey=G::screenHeight*0.5f-sinf(a)*G::screenHeight*0.45f;
                    ex=Math::Clamp(ex,50.f,G::screenWidth-50.f); ey=Math::Clamp(ey,50.f,G::screenHeight-50.f);
                    float da=Math::Clamp(1.f-d/3000.f,0.2f,1.f);
                    Renderer::DrawFilledCircle(ex,ey,8+da*5,IM_COL32(255,60,60,(int)(da*255)));
                }
            }
        }

        if (cfg.lastHitHelper) {
            int mx=SDK::GetMaxEntities(); if(mx<=0||mx>20000)mx=2048;
            uint8_t lt=lh->GetTeam();
            auto hero=reinterpret_cast<C_DOTA_BaseNPC_Hero*>(lh);
            float dmg=50.f; int pa=hero->GetPrimaryAttribute();
            switch(pa){case 0:dmg+=hero->GetStrengthTotal();break;case 1:dmg+=hero->GetAgilityTotal();break;
                       case 2:dmg+=hero->GetIntellectTotal();break;default:dmg+=(hero->GetStrengthTotal()+hero->GetAgilityTotal()+hero->GetIntellectTotal())*0.7f;break;}
            float ar=(float)npc->GetAttackRange();
            for (int i=0;i<mx;i++) {
                auto e=SDK::GetEntityByIndex(i); if(!e||!e->IsAlive())continue;
                auto cn=e->GetClassName(); if(!cn||!strstr(cn,"Creep"))continue;
                if(e->GetTeam()==lt)continue;
                auto cp=e->GetPosition(); if(hp.Distance2D(cp)>ar+400)continue;
                int chp=e->GetHealth(); if(chp<=0)continue;
                if((float)chp<=dmg) {
                    Vector2 sp; if(Math::WorldToScreen(cp,sp,vm,G::screenWidth,G::screenHeight)) {
                        float pulse=(sinf(GetTickCount64()/200.f)+1.f)*0.5f;
                        Renderer::DrawCircle(sp.x,sp.y,15,IM_COL32(50,255,50,(int)(pulse*130+125)),16,2.5f);
                        char ht[16]; snprintf(ht,16,"%d",chp);
                        Renderer::DrawText(sp.x,sp.y-10,IM_COL32(50,255,50,(int)(pulse*130+125)),ht,12,true);
                    }
                }
            }
        }
    }
}
