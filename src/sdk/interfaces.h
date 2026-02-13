#pragma once
#include "source2sdk.h"
#include <cstdint>
#include <vector>

namespace SDK {
    bool Initialize();

    inline uintptr_t pGameEntitySystem = 0;
    inline uintptr_t pGameRules        = 0;
    inline uintptr_t pCameraInstance   = 0;
    inline uintptr_t pLocalPlayer      = 0;
    inline uintptr_t pViewMatrix       = 0;

    C_DOTA_BaseNPC_Hero*    GetLocalHero();
    C_DOTAPlayerController* GetLocalPlayerController();
    Matrix4x4               GetViewMatrix();
    float                   GetGameTime();
    bool                    IsGamePaused();
    int                     GetMaxEntities();

    C_BaseEntity*    GetEntityByIndex(int index);
    C_BaseEntity*    GetEntityFromHandle(uint32_t handle);

    void CollectHeroes(std::vector<HeroData>& outHeroes);
    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& outAbilities);
    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& outItems);
}
