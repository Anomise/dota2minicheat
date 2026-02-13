#pragma once
#include "source2sdk.h"
#include <cstdint>
#include <vector>
#include <string>

namespace SDK {
    bool Initialize();
    void OnFrame();

    // Pointers
    inline uintptr_t pGameEntitySystem  = 0;
    inline uintptr_t pGameRules         = 0;
    inline uintptr_t pLocalPlayer       = 0;
    inline uintptr_t pViewMatrix        = 0;
    inline uintptr_t pCameraInstance    = 0;

    // Module bases (cached)
    inline uintptr_t clientBase   = 0;
    inline uintptr_t clientSize   = 0;
    inline uintptr_t engineBase   = 0;
    inline uintptr_t engineSize   = 0;

    // State
    inline bool bInGame = false;
    inline Matrix4x4 cachedViewMatrix = {};

    // Interface finding (Source 2 CreateInterface)
    void* GetInterface(const char* moduleName, const char* ifaceName);

    // Accessors
    C_DOTA_BaseNPC_Hero*    GetLocalHero();
    C_DOTAPlayerController* GetLocalPlayerController();
    Matrix4x4               GetViewMatrix();
    float                   GetGameTime();
    bool                    IsGamePaused();
    bool                    IsInGame();
    int                     GetMaxEntities();

    // Entity access
    C_BaseEntity* GetEntityByIndex(int index);
    C_BaseEntity* GetEntityFromHandle(uint32_t handle);

    // Collection
    void CollectHeroes(std::vector<HeroData>& outHeroes);
    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& outAbilities);
    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& outItems);
}
