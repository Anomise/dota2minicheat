#include "interfaces.h"
#include "../utils/memory.h"
#include <cstring>
#include <algorithm>

#define LOG(fmt, ...) printf("[SDK] " fmt "\n", ##__VA_ARGS__)

namespace SDK {

    // ============================================
    // Source 2 CreateInterface
    // Same approach as Andromeda
    // ============================================
    void* GetInterface(const char* moduleName, const char* ifaceName) {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (!hMod) return nullptr;

        using CreateInterfaceFn = void*(*)(const char*, int*);
        auto fn = reinterpret_cast<CreateInterfaceFn>(
            GetProcAddress(hMod, "CreateInterface"));
        if (!fn) return nullptr;

        int ret = 0;
        void* iface = fn(ifaceName, &ret);
        if (iface) {
            LOG("Interface %s::%s = %p", moduleName, ifaceName, iface);
        }
        return iface;
    }

    // ============================================
    // Get pointer from vtable function
    // Many Source 2 getters are just: mov rax, [rip+X]; ret
    // ============================================
    static uintptr_t GetPtrFromVFunc(void* iface, int index) {
        if (!iface) return 0;
        auto vtable = *reinterpret_cast<uintptr_t**>(iface);
        if (!vtable) return 0;
        auto func = vtable[index];
        if (!func) return 0;

        // Check for: 48 8B 05 XX XX XX XX (mov rax, [rip+XX])
        auto bytes = reinterpret_cast<uint8_t*>(func);
        if (bytes[0] == 0x48 && bytes[1] == 0x8B && bytes[2] == 0x05) {
            int32_t offset = *reinterpret_cast<int32_t*>(func + 3);
            uintptr_t addr = func + 7 + offset;
            return addr;
        }

        // Check for: 48 8D 05 XX XX XX XX (lea rax, [rip+XX])
        if (bytes[0] == 0x48 && bytes[1] == 0x8D && bytes[2] == 0x05) {
            int32_t offset = *reinterpret_cast<int32_t*>(func + 3);
            uintptr_t addr = func + 7 + offset;
            return addr;
        }

        // Check for: 48 89 ?? 48 8B 05 XX XX XX XX
        for (int i = 0; i < 32; i++) {
            if (bytes[i] == 0x48 && bytes[i+1] == 0x8B && bytes[i+2] == 0x05) {
                int32_t offset = *reinterpret_cast<int32_t*>(func + i + 3);
                uintptr_t addr = func + i + 7 + offset;
                return addr;
            }
        }

        return 0;
    }

    // ============================================
    // Call vtable function that returns a pointer
    // ============================================
    static void* CallVFunc(void* iface, int index) {
        if (!iface) return nullptr;
        auto vtable = *reinterpret_cast<void***>(iface);
        if (!vtable) return nullptr;
        using Fn = void*(*)(void*);
        auto func = reinterpret_cast<Fn>(vtable[index]);
        if (!func) return nullptr;
        return func(iface);
    }

    // ============================================
    // Initialize - Andromeda-style
    // ============================================
    bool Initialize() {
        LOG("Initializing...");

        // Wait for client.dll
        int waitCount = 0;
        while (!(clientBase = Memory::GetModuleBase("client.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) {
                LOG("TIMEOUT: client.dll not found");
                return false;
            }
        }
        clientSize = Memory::GetModuleSize("client.dll");
        LOG("client.dll: 0x%llX (size: 0x%llX)", clientBase, clientSize);

        while (!(engineBase = Memory::GetModuleBase("engine2.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) {
                LOG("TIMEOUT: engine2.dll not found");
                return false;
            }
        }
        engineSize = Memory::GetModuleSize("engine2.dll");
        LOG("engine2.dll: 0x%llX", engineBase);

        // Wait for game to fully load
        Sleep(8000);

        // ==========================================
        // Method 1: Interface-based (like Andromeda)
        // ==========================================

        // GameResourceService -> GetGameEntitySystem
        auto pGameResourceService = GetInterface("engine2.dll", "GameResourceServiceClientV001");
        if (pGameResourceService) {
            // GetGameEntitySystem is typically vfunc index 1
            auto gesPtr = CallVFunc(pGameResourceService, 1);
            if (gesPtr) {
                pGameEntitySystem = reinterpret_cast<uintptr_t>(gesPtr);
                LOG("GameEntitySystem (via interface): 0x%llX", pGameEntitySystem);
            }
        }

        // Source2Client
        auto pSource2Client = GetInterface("client.dll", "Source2Client002");
        if (pSource2Client) {
            LOG("Source2Client: %p", pSource2Client);
        }

        // ==========================================
        // Method 2: Pattern scanning fallbacks
        // ==========================================

        if (!pGameEntitySystem) {
            // Pattern: 48 8B 1D ?? ?? ?? ?? 48 85 DB 74
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 1D ?? ?? ?? ?? 48 85 DB 74");
            if (pat) {
                auto addr = Memory::ResolveRelative(pat, 3, 7);
                pGameEntitySystem = Memory::Read<uintptr_t>(addr);
                LOG("GameEntitySystem (via pattern): 0x%llX", pGameEntitySystem);
            }
        }

        // LocalPlayer / PlayerController
        {
            // Andromeda uses CDOTAPlayerController patterns
            auto pat = Memory::FindPattern("client.dll",
                "48 89 05 ?? ?? ?? ?? 48 8B 4E");
            if (pat) {
                pLocalPlayer = Memory::ResolveRelative(pat, 3, 7);
                LOG("LocalPlayer: 0x%llX", pLocalPlayer);
            }

            if (!pLocalPlayer) {
                // Alt pattern
                auto pat2 = Memory::FindPattern("client.dll",
                    "48 89 3D ?? ?? ?? ?? 48 8B 5C 24");
                if (pat2) {
                    pLocalPlayer = Memory::ResolveRelative(pat2, 3, 7);
                    LOG("LocalPlayer (alt): 0x%llX", pLocalPlayer);
                }
            }
        }

        // GameRules
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 48");
            if (pat) {
                pGameRules = Memory::ResolveRelative(pat, 3, 7);
                LOG("GameRules: 0x%llX", pGameRules);
            }

            if (!pGameRules) {
                auto pat2 = Memory::FindPattern("client.dll",
                    "48 89 0D ?? ?? ?? ?? 48 89 41");
                if (pat2) {
                    pGameRules = Memory::ResolveRelative(pat2, 3, 7);
                    LOG("GameRules (alt): 0x%llX", pGameRules);
                }
            }
        }

        // ViewMatrix
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 8D 05 ?? ?? ?? ?? 48 C7 05");
            if (pat) {
                pViewMatrix = Memory::ResolveRelative(pat, 3, 7);
                LOG("ViewMatrix: 0x%llX", pViewMatrix);
            }

            // Andromeda-style: ViewRender
            if (!pViewMatrix) {
                auto pat2 = Memory::FindPattern("client.dll",
                    "48 89 05 ?? ?? ?? ?? 48 8B C8 48 85 C0 74 ?? 48 8B 10");
                if (pat2) {
                    auto viewRenderAddr = Memory::ResolveRelative(pat2, 3, 7);
                    auto viewRender = Memory::Read<uintptr_t>(viewRenderAddr);
                    if (viewRender) {
                        // VMatrix is at offset in ViewRender
                        pViewMatrix = viewRender + 0x588; // Typical offset, may vary
                        LOG("ViewMatrix (via ViewRender): 0x%llX", pViewMatrix);
                    }
                }
            }
        }

        // Camera (CDOTA_Camera)
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 8D 05 ?? ?? ?? ?? 48 89 01 F3 0F 10");
            if (pat) {
                pCameraInstance = Memory::ResolveRelative(pat, 3, 7);
                LOG("Camera: 0x%llX", pCameraInstance);
            }
        }

        // ==========================================
        // Verify
        // ==========================================
        LOG("=== Init Results ===");
        LOG("  EntitySystem: 0x%llX %s", pGameEntitySystem, pGameEntitySystem ? "OK" : "FAIL");
        LOG("  LocalPlayer:  0x%llX %s", pLocalPlayer, pLocalPlayer ? "OK" : "FAIL");
        LOG("  ViewMatrix:   0x%llX %s", pViewMatrix, pViewMatrix ? "OK" : "FAIL");
        LOG("  GameRules:    0x%llX %s", pGameRules, pGameRules ? "OK" : "FAIL");
        LOG("  Camera:       0x%llX %s", pCameraInstance, pCameraInstance ? "OK" : "FAIL");

        return pGameEntitySystem != 0;
    }

    // ============================================
    // OnFrame
    // ============================================
    void OnFrame() {
        bInGame = false;
        auto ctrl = GetLocalPlayerController();
        if (!ctrl) return;
        auto hero = GetLocalHero();
        if (!hero) return;
        bInGame = true;

        if (pViewMatrix) {
            cachedViewMatrix = Memory::Read<Matrix4x4>(pViewMatrix);
        }
    }

    // ============================================
    // Entity access — Andromeda style
    // Key difference: entity identity structure
    // ============================================
    C_BaseEntity* GetEntityByIndex(int index) {
        if (!pGameEntitySystem || index < 0) return nullptr;

        // In Source 2 (Andromeda approach):
        // GameEntitySystem + 0x10 = entity list base
        // Each "chunk" holds 512 entries
        // chunk = index / 512
        // slot = index % 512
        // chunk_ptr = entityList[chunk * 8]
        // entity_identity = chunk_ptr + slot * 0x78
        // entity_ptr = *(entity_identity + 0x0) or entity_identity itself contains the entity

        auto entityList = Memory::Read<uintptr_t>(
            pGameEntitySystem + Offsets::EntitySystem::EntityList);
        if (!entityList) return nullptr;

        int chunk = index >> 9;  // / 512
        int slot = index & 0x1FF; // % 512

        // Read chunk pointer
        auto chunkAddr = entityList + (uintptr_t)chunk * 0x8;
        if (IsBadReadPtr(reinterpret_cast<void*>(chunkAddr), 8)) return nullptr;
        auto chunkPtr = Memory::Read<uintptr_t>(chunkAddr);
        if (!chunkPtr) return nullptr;

        // Read entity identity
        auto identityAddr = chunkPtr + (uintptr_t)slot * 0x78;
        if (IsBadReadPtr(reinterpret_cast<void*>(identityAddr), 8)) return nullptr;

        // In Andromeda: they read the entity pointer from identity
        // CEntityIdentity layout:
        //   +0x00 = entity pointer (CEntityInstance*)
        //   +0x08 = ...
        //   +0x18 = designer name
        //   +0x20 = ...
        auto entityPtr = Memory::Read<uintptr_t>(identityAddr);
        if (!entityPtr || entityPtr < 0x10000) return nullptr;
        if (IsBadReadPtr(reinterpret_cast<void*>(entityPtr), 16)) return nullptr;

        return reinterpret_cast<C_BaseEntity*>(entityPtr);
    }

    C_BaseEntity* GetEntityFromHandle(uint32_t handle) {
        if (handle == 0xFFFFFFFF || handle == 0) return nullptr;
        // Andromeda: handle & 0x7FFF = entity index
        int index = handle & 0x7FFF;
        return GetEntityByIndex(index);
    }

    // ============================================
    // Local player — Andromeda style
    // ============================================
    C_DOTAPlayerController* GetLocalPlayerController() {
        if (!pLocalPlayer) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(pLocalPlayer);
        if (!ptr || ptr < 0x10000) return nullptr;
        if (IsBadReadPtr(reinterpret_cast<void*>(ptr), 16)) return nullptr;
        return reinterpret_cast<C_DOTAPlayerController*>(ptr);
    }

    C_DOTA_BaseNPC_Hero* GetLocalHero() {
        auto ctrl = GetLocalPlayerController();
        if (!ctrl) return nullptr;

        auto heroHandle = ctrl->GetAssignedHeroHandle();
        if (heroHandle == 0xFFFFFFFF || heroHandle == 0) return nullptr;

        auto ent = GetEntityFromHandle(heroHandle);
        if (!ent) return nullptr;

        return reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);
    }

    Matrix4x4 GetViewMatrix() { return cachedViewMatrix; }

    float GetGameTime() {
        if (!pGameRules) return 0.f;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules || IsBadReadPtr(reinterpret_cast<void*>(rules), 8)) return 0.f;
        return Memory::Read<float>(rules + Offsets::GameRules::GameTime);
    }

    bool IsGamePaused() {
        if (!pGameRules) return false;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules || IsBadReadPtr(reinterpret_cast<void*>(rules), 8)) return false;
        return Memory::Read<bool>(rules + Offsets::GameRules::IsPaused);
    }

    bool IsInGame() { return bInGame; }

    int GetMaxEntities() {
        if (!pGameEntitySystem) return 0;
        auto val = Memory::Read<int>(
            pGameEntitySystem + Offsets::EntitySystem::HighestEntityIndex);
        return (val > 0 && val < 32768) ? val : 0;
    }

    // ============================================
    // Class name reading - Andromeda style
    // CEntityIdentity is at entity + 0x10
    // CEntityIdentity->m_designerName at +0x20
    // CEntityIdentity->m_name at +0x18
    // ============================================

    // ============================================
    // Ability collection
    // ============================================
    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& out) {
        out.clear();
        if (!npc || !npc->IsValid()) return;

        int cnt = npc->GetAbilityCount();
        if (cnt <= 0 || cnt > 35) return;

        for (int i = 0; i < cnt; i++) {
            auto handle = npc->GetAbilityHandle(i);
            if (handle == 0xFFFFFFFF || handle == 0) continue;

            auto ent = GetEntityFromHandle(handle);
            if (!ent || !ent->IsValid()) continue;

            auto ab = reinterpret_cast<C_DOTABaseAbility*>(ent);

            AbilityInfo info;
            info.level       = ab->GetLevel();
            info.cooldown    = ab->GetCooldown();
            info.cooldownMax = ab->GetCooldownLength();
            info.manaCost    = ab->GetManaCost();
            info.charges     = ab->GetCharges();
            info.chargeRestore = ab->GetChargeRestoreTime();
            info.isHidden    = ab->IsHidden();
            info.isActivated = ab->IsActivated();
            info.isToggled   = ab->IsToggled();
            info.inPhase     = ab->IsInAbilityPhase();

            auto cn = ent->GetClassName();
            if (cn && strlen(cn) > 0 && strlen(cn) < 256)
                info.name = cn;
            else
                info.name = "ability_" + std::to_string(i);

            info.isUltimate = (ab->GetMaxLevel() > 0 && ab->GetMaxLevel() <= 3 && i >= 5);

            if (info.isHidden && info.level == 0) continue;

            out.push_back(info);
        }
    }

    // ============================================
    // Item collection
    // ============================================
    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& out) {
        out.clear();
        if (!npc || !npc->IsValid()) return;

        for (int slot = 0; slot < 16; slot++) {
            auto handle = npc->GetItemHandle(slot);
            if (handle == 0xFFFFFFFF || handle == 0) continue;

            auto ent = GetEntityFromHandle(handle);
            if (!ent || !ent->IsValid()) continue;

            auto item = reinterpret_cast<C_DOTA_Item*>(ent);

            ItemInfo info;
            info.slot       = slot;
            info.charges    = item->GetCurrentCharges();
            info.cooldown   = item->GetCooldown();
            info.cooldownMax = item->GetCooldownLength();

            auto cn = ent->GetClassName();
            if (cn && strlen(cn) > 0 && strlen(cn) < 256)
                info.name = cn;
            else
                info.name = "item_" + std::to_string(slot);

            out.push_back(info);
        }
    }

    // ============================================
    // Hero collection
    // ============================================
    void CollectHeroes(std::vector<HeroData>& out) {
        out.clear();
        if (!bInGame) return;

        int maxEnt = GetMaxEntities();
        if (maxEnt <= 0) return;

        int limit = (std::min)(maxEnt, 512);

        for (int i = 0; i < limit; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent || !ent->IsValid()) continue;

            auto className = ent->GetClassName();
            if (!className || strlen(className) < 5) continue;

            // Andromeda checks for "C_DOTA_Unit_Hero" in class name
            if (strstr(className, "Hero") == nullptr) continue;
            if (strstr(className, "C_DOTA_Unit_Hero") == nullptr &&
                strstr(className, "CDOTA_Unit_Hero") == nullptr)
                continue;

            auto npc  = reinterpret_cast<C_DOTA_BaseNPC*>(ent);
            auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);

            HeroData d;
            d.entity      = hero;
            d.pos         = hero->GetPosition();
            d.team        = hero->GetTeam();
            d.health      = ent->GetHealth();
            d.maxHP       = ent->GetMaxHealth();
            d.mana        = npc->GetMana();
            d.maxMana     = npc->GetMaxMana();
            d.hpRegen     = npc->GetHealthRegen();
            d.mpRegen     = npc->GetManaRegen();
            d.level       = npc->GetLevel();
            d.heroID      = hero->GetHeroID();
            d.playerID    = hero->GetPlayerID();
            d.alive       = ent->IsAlive();
            d.illusion    = npc->IsIllusion() || hero->IsReplicating();
            d.moveSpeed   = npc->GetMoveSpeed();
            d.attackRange = npc->GetAttackRange();
            d.primaryAttr = hero->GetPrimaryAttributeName();

            auto un = npc->GetUnitName();
            d.name = (un && strlen(un) < 256) ? un : className;

            d.str   = hero->GetStrengthTotal();
            d.agi   = hero->GetAgilityTotal();
            d.intel = hero->GetIntellectTotal();

            d.stunned     = npc->IsStunned();
            d.silenced    = npc->IsSilenced();
            d.hexed       = npc->IsHexed();
            d.rooted      = npc->IsRooted();
            d.invisible   = npc->IsInvisible();
            d.invulnerable = npc->IsInvulnerable();
            d.magicImmune = npc->IsMagicImmune();
            d.disarmed    = npc->IsDisarmed();
            d.muted       = npc->IsMuted();

            CollectAbilities(npc, d.abilities);
            CollectItems(npc, d.items);

            out.push_back(std::move(d));
        }
    }
}
