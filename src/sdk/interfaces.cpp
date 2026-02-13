#include "interfaces.h"
#include "../utils/memory.h"
#include "../utils/pattern_scan.h"
#include <cstring>
#include <algorithm>

namespace SDK {

    bool Initialize() {
        int waitCount = 0;
        while (!Memory::GetModuleBase(Offsets::CLIENT_DLL)) {
            Sleep(500);
            if (++waitCount > 60) return false;
        }
        while (!Memory::GetModuleBase(Offsets::ENGINE_DLL)) {
            Sleep(500);
            if (++waitCount > 60) return false;
        }
        Sleep(5000);

        auto patGES = Memory::FindPattern(Offsets::CLIENT_DLL, Offsets::Patterns::GameEntitySystem);
        if (patGES) pGameEntitySystem = Memory::Read<uintptr_t>(Memory::ResolveRelative(patGES, 3, 7));

        auto patCam = Memory::FindPattern(Offsets::CLIENT_DLL, Offsets::Patterns::CameraInstance);
        if (patCam) pCameraInstance = Memory::ResolveRelative(patCam, 3, 7);

        auto patGR = Memory::FindPattern(Offsets::CLIENT_DLL, Offsets::Patterns::GameRules);
        if (patGR) pGameRules = Memory::ResolveRelative(patGR, 3, 7);

        auto patVM = Memory::FindPattern(Offsets::CLIENT_DLL, Offsets::Patterns::ViewMatrix);
        if (patVM) pViewMatrix = Memory::ResolveRelative(patVM, 3, 7);

        auto patLP = Memory::FindPattern(Offsets::CLIENT_DLL, Offsets::Patterns::LocalPlayer);
        if (patLP) pLocalPlayer = Memory::ResolveRelative(patLP, 3, 7);

#ifdef _DEBUG
        printf("[SDK] GES=0x%llX CAM=0x%llX GR=0x%llX VM=0x%llX LP=0x%llX\n",
            pGameEntitySystem, pCameraInstance, pGameRules, pViewMatrix, pLocalPlayer);
#endif

        return pGameEntitySystem != 0;
    }

    C_DOTA_BaseNPC_Hero* GetLocalHero() {
        if (!pLocalPlayer) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(pLocalPlayer);
        if (!ptr) return nullptr;
        auto controller = reinterpret_cast<C_DOTAPlayerController*>(ptr);
        auto heroHandle = controller->GetAssignedHeroHandle();
        if (heroHandle == 0xFFFFFFFF) return nullptr;
        auto heroEnt = GetEntityFromHandle(heroHandle);
        if (!heroEnt) return nullptr;
        return reinterpret_cast<C_DOTA_BaseNPC_Hero*>(heroEnt);
    }

    C_DOTAPlayerController* GetLocalPlayerController() {
        if (!pLocalPlayer) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(pLocalPlayer);
        if (!ptr) return nullptr;
        return reinterpret_cast<C_DOTAPlayerController*>(ptr);
    }

    Matrix4x4 GetViewMatrix() {
        if (!pViewMatrix) return {};
        return Memory::Read<Matrix4x4>(pViewMatrix);
    }

    float GetGameTime() {
        if (!pGameRules) return 0.f;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules) return 0.f;
        return Memory::Read<float>(rules + Offsets::GameRules::GameTime);
    }

    bool IsGamePaused() {
        if (!pGameRules) return false;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules) return false;
        return Memory::Read<bool>(rules + Offsets::GameRules::IsPaused);
    }

    int GetMaxEntities() {
        if (!pGameEntitySystem) return 0;
        return Memory::Read<int>(pGameEntitySystem + Offsets::EntitySystem::HighestEntityIndex);
    }

    C_BaseEntity* GetEntityByIndex(int index) {
        if (!pGameEntitySystem) return nullptr;
        auto entityList = Memory::Read<uintptr_t>(pGameEntitySystem + Offsets::EntitySystem::EntityList);
        if (!entityList) return nullptr;
        int chunk = index >> 9;
        int slot  = index & 0x1FF;
        auto chunkPtr = Memory::Read<uintptr_t>(entityList + static_cast<uintptr_t>(chunk) * 0x8);
        if (!chunkPtr) return nullptr;
        auto identityPtr = Memory::Read<uintptr_t>(chunkPtr + static_cast<uintptr_t>(slot) * 0x78);
        if (!identityPtr) return nullptr;
        return reinterpret_cast<C_BaseEntity*>(identityPtr);
    }

    C_BaseEntity* GetEntityFromHandle(uint32_t handle) {
        if (handle == 0xFFFFFFFF || handle == 0) return nullptr;
        int index = handle & 0x7FFF;
        return GetEntityByIndex(index);
    }

    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& outAbilities) {
        outAbilities.clear();
        if (!npc) return;
        int count = npc->GetAbilityCount();
        if (count <= 0 || count > 35) return;
        for (int i = 0; i < count; i++) {
            auto handle = npc->GetAbilityHandle(i);
            if (handle == 0xFFFFFFFF || handle == 0) continue;
            auto abilityEnt = GetEntityFromHandle(handle);
            if (!abilityEnt) continue;
            auto ability = reinterpret_cast<C_DOTABaseAbility*>(abilityEnt);
            AbilityInfo info;
            info.level = ability->GetLevel();
            info.cooldown = ability->GetCooldown();
            info.cooldownMax = ability->GetCooldownLength();
            info.manaCost = ability->GetManaCost();
            info.charges = ability->GetCharges();
            info.chargeRestore = ability->GetChargeRestoreTime();
            info.isHidden = ability->IsHidden();
            info.isActivated = ability->IsActivated();
            info.isToggled = ability->IsToggled();
            info.inPhase = ability->IsInAbilityPhase();
            auto abilityName = abilityEnt->GetClassName();
            if (abilityName && strlen(abilityName) > 0) info.name = abilityName;
            info.isUltimate = (ability->GetMaxLevel() <= 3 && i >= 5);
            if (info.isHidden && info.level == 0) continue;
            outAbilities.push_back(info);
        }
    }

    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& outItems) {
        outItems.clear();
        if (!npc) return;
        for (int slot = 0; slot < 16; slot++) {
            auto handle = npc->GetItemHandle(slot);
            if (handle == 0xFFFFFFFF || handle == 0) continue;
            auto itemEnt = GetEntityFromHandle(handle);
            if (!itemEnt) continue;
            auto item = reinterpret_cast<C_DOTA_Item*>(itemEnt);
            ItemInfo info;
            info.slot = slot;
            info.charges = item->GetCurrentCharges();
            info.cooldown = item->GetCooldown();
            info.cooldownMax = item->GetCooldownLength();
            auto itemName = itemEnt->GetClassName();
            if (itemName && strlen(itemName) > 0) info.name = itemName;
            outItems.push_back(info);
        }
    }

    void CollectHeroes(std::vector<HeroData>& outHeroes) {
        outHeroes.clear();
        int maxEnt = GetMaxEntities();
        if (maxEnt <= 0 || maxEnt > 20000) maxEnt = 2048;
        for (int i = 0; i < maxEnt; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent) continue;
            auto className = ent->GetClassName();
            if (!className) continue;
            if (strstr(className, "CDOTA_Unit_Hero") == nullptr &&
                strstr(className, "C_DOTA_Unit_Hero") == nullptr) continue;
            auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(ent);
            auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);
            HeroData data;
            data.entity = hero;
            data.pos = hero->GetPosition();
            data.team = hero->GetTeam();
            data.health = hero->GetHealth();
            data.maxHP = hero->GetMaxHealth();
            data.mana = hero->GetMana();
            data.maxMana = hero->GetMaxMana();
            data.hpRegen = npc->GetHealthRegen();
            data.mpRegen = npc->GetManaRegen();
            data.level = npc->GetLevel();
            data.heroID = hero->GetHeroID();
            data.playerID = hero->GetPlayerID();
            data.alive = hero->IsAlive();
            data.illusion = npc->IsIllusion() || hero->IsReplicating();
            data.moveSpeed = npc->GetMoveSpeed();
            data.attackRange = npc->GetAttackRange();
            data.name = npc->GetUnitName();
            data.primaryAttr = hero->GetPrimaryAttributeName();
            data.str = hero->GetStrengthTotal();
            data.agi = hero->GetAgilityTotal();
            data.intel = hero->GetIntellectTotal();
            data.stunned = npc->IsStunned();
            data.silenced = npc->IsSilenced();
            data.hexed = npc->IsHexed();
            data.rooted = npc->IsRooted();
            data.invisible = npc->IsInvisible();
            data.invulnerable = npc->IsInvulnerable();
            data.magicImmune = npc->IsMagicImmune();
            data.disarmed = npc->IsDisarmed();
            data.muted = npc->IsMuted();
            CollectAbilities(npc, data.abilities);
            CollectItems(npc, data.items);
            outHeroes.push_back(std::move(data));
        }
    }
}
