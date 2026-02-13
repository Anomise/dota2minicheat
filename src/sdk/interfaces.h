#pragma once
#include "source2sdk.h"
#include <cstdint>
#include <vector>
#include <string>

namespace SDK {
    bool Initialize();
    void OnFrame(); // вызывать каждый кадр для обновления кешей

    // Указатели (обновляются в Initialize и OnFrame)
    inline uintptr_t pGameEntitySystem = 0;
    inline uintptr_t pGameRules        = 0;
    inline uintptr_t pLocalPlayer      = 0;
    inline uintptr_t pViewMatrix       = 0;
    inline uintptr_t pCameraInstance   = 0;

    // Кешированные данные (обновляются в OnFrame)
    inline bool       bInGame          = false;
    inline Matrix4x4  cachedViewMatrix = {};

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

    // Interface registry helper
    uintptr_t FindInterface(const char* moduleName, const char* interfaceName);
}
