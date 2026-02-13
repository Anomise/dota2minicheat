#include "interfaces.h"
#include "../utils/memory.h"
#include <cstring>
#include <algorithm>

#define LOG(fmt, ...) printf("[SDK] " fmt "\n", ##__VA_ARGS__)

namespace SDK {

    void* GetInterface(const char* moduleName, const char* ifaceName) {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (!hMod) return nullptr;
        using CreateInterfaceFn = void*(*)(const char*, int*);
        auto fn = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hMod, "CreateInterface"));
        if (!fn) return nullptr;
        int ret = 0;
        void* iface = fn(ifaceName, &ret);
        if (iface) LOG("Interface %s::%s = %p", moduleName, ifaceName, iface);
        return iface;
    }

    static void* CallVFunc(void* iface, int index) {
        if (!iface) return nullptr;
        auto vtable = *reinterpret_cast<void***>(iface);
        if (!vtable) return nullptr;
        using Fn = void*(*)(void*);
        return reinterpret_cast<Fn>(vtable[index])(iface);
    }

    // ============================================
    // Scan for camera distance ConVar or direct pointer
    // ============================================
    static uintptr_t FindCameraDistance() {
        // Method 1: Find "dota_camera_distance" ConVar
        auto pat = Memory::FindPattern("client.dll",
            "F3 0F 10 05 ?? ?? ?? ?? F3 0F 5C C1 F3 0F 58 05");
        if (pat) {
            auto addr = Memory::ResolveRelative(pat, 4, 8);
            LOG("Camera distance (convar pattern): 0x%llX", addr);
            return addr;
        }

        // Method 2: Search for default value 1200.f = 0x44960000
        // in client.dll data sections near camera-related code
        auto pat2 = Memory::FindPattern("client.dll",
            "F3 0F 10 ?? ?? ?? ?? ?? 0F 2F ?? F3 0F 10");
        if (pat2) {
            auto addr = Memory::ResolveRelative(pat2, 4, 8);
            LOG("Camera distance (pattern2): 0x%llX", addr);
            return addr;
        }

        // Method 3: ConVar system
        auto pat3 = Memory::FindPattern("client.dll",
            "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 10");
        if (pat3) {
            // This might be near camera ConVar registration
            LOG("Camera convar area found at: 0x%llX", pat3);
        }

        return 0;
    }

    // ============================================
    // Find ViewMatrix properly
    // Source 2 stores it in CRenderGameSystem
    // ============================================
    static uintptr_t FindViewMatrix() {
        // Method 1: Standard pattern
        auto pat = Memory::FindPattern("client.dll",
            "48 8D 05 ?? ?? ?? ?? 48 C7 05");
        if (pat) {
            auto addr = Memory::ResolveRelative(pat, 3, 7);
            LOG("ViewMatrix (pattern1): 0x%llX", addr);
            return addr;
        }

        // Method 2: ViewRender pattern
        auto pat2 = Memory::FindPattern("client.dll",
            "48 89 05 ?? ?? ?? ?? 48 8B C8 48 85 C0");
        if (pat2) {
            auto viewRenderGlobal = Memory::ResolveRelative(pat2, 3, 7);
            LOG("ViewRender global: 0x%llX", viewRenderGlobal);
            return viewRenderGlobal; // We'll handle offset in GetViewMatrix
        }

        // Method 3: VMatrix from engine
        auto pat3 = Memory::FindPattern("engine2.dll",
            "48 8D 0D ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00");
        if (pat3) {
            auto addr = Memory::ResolveRelative(pat3, 3, 7);
            LOG("ViewMatrix (engine): 0x%llX", addr);
            return addr;
        }

        return 0;
    }

    // ============================================
    // Dump first N entities for debugging
    // ============================================
    static void DebugDumpEntities(int count) {
        LOG("--- Entity Dump (first %d) ---", count);
        int found = 0;
        for (int i = 0; i < count && found < 20; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent) continue;

            auto cn = ent->GetClassName();
            auto en = ent->GetEntityName();

            if (cn && strlen(cn) > 0) {
                int hp = ent->GetHealth();
                int team = ent->GetTeam();
                auto pos = ent->GetPosition();
                LOG("  [%d] class='%s' name='%s' hp=%d team=%d pos=(%.0f,%.0f,%.0f)",
                    i, cn, en ? en : "?", hp, team, pos.x, pos.y, pos.z);
                found++;
            }
        }
        LOG("--- End dump (found %d) ---", found);
    }

    // ============================================
    // Initialize
    // ============================================
    bool Initialize() {
        LOG("Initializing...");

        int waitCount = 0;
        while (!(clientBase = Memory::GetModuleBase("client.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) { LOG("TIMEOUT: client.dll"); return false; }
        }
        clientSize = Memory::GetModuleSize("client.dll");
        LOG("client.dll: 0x%llX (size: 0x%llX)", clientBase, clientSize);

        while (!(engineBase = Memory::GetModuleBase("engine2.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) { LOG("TIMEOUT: engine2.dll"); return false; }
        }
        engineSize = Memory::GetModuleSize("engine2.dll");
        LOG("engine2.dll: 0x%llX", engineBase);

        Sleep(8000);

        // ==========================================
        // GameEntitySystem via CreateInterface
        // ==========================================
        auto pGameResourceService = GetInterface("engine2.dll", "GameResourceServiceClientV001");
        if (pGameResourceService) {
            auto gesPtr = CallVFunc(pGameResourceService, 1);
            if (gesPtr) {
                pGameEntitySystem = reinterpret_cast<uintptr_t>(gesPtr);
                LOG("GameEntitySystem (interface): 0x%llX", pGameEntitySystem);
            }
        }

        if (!pGameEntitySystem) {
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 1D ?? ?? ?? ?? 48 85 DB 74");
            if (pat) {
                auto addr = Memory::ResolveRelative(pat, 3, 7);
                pGameEntitySystem = Memory::Read<uintptr_t>(addr);
                LOG("GameEntitySystem (pattern): 0x%llX", pGameEntitySystem);
            }
        }

        // Source2Client
        auto pSource2Client = GetInterface("client.dll", "Source2Client002");
        if (pSource2Client) {
            LOG("Source2Client: %p", pSource2Client);
        }

        // ==========================================
        // LocalPlayer
        // ==========================================
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 89 05 ?? ?? ?? ?? 48 8B 4E");
            if (pat) {
                pLocalPlayer = Memory::ResolveRelative(pat, 3, 7);
                LOG("LocalPlayer: 0x%llX", pLocalPlayer);
            }
        }
        if (!pLocalPlayer) {
            auto pat2 = Memory::FindPattern("client.dll",
                "48 89 3D ?? ?? ?? ?? 48 8B 5C 24");
            if (pat2) {
                pLocalPlayer = Memory::ResolveRelative(pat2, 3, 7);
                LOG("LocalPlayer (alt): 0x%llX", pLocalPlayer);
            }
        }
        if (!pLocalPlayer) {
            auto pat3 = Memory::FindPattern("client.dll",
                "48 89 1D ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3");
            if (pat3) {
                pLocalPlayer = Memory::ResolveRelative(pat3, 3, 7);
                LOG("LocalPlayer (alt2): 0x%llX", pLocalPlayer);
            }
        }

        // ==========================================
        // GameRules
        // ==========================================
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 48");
            if (pat) {
                pGameRules = Memory::ResolveRelative(pat, 3, 7);
                LOG("GameRules: 0x%llX", pGameRules);
            }
        }
        if (!pGameRules) {
            auto pat2 = Memory::FindPattern("client.dll",
                "48 89 0D ?? ?? ?? ?? 48 89 41");
            if (pat2) {
                pGameRules = Memory::ResolveRelative(pat2, 3, 7);
                LOG("GameRules (alt): 0x%llX", pGameRules);
            }
        }

        // ==========================================
        // ViewMatrix
        // ==========================================
        pViewMatrix = FindViewMatrix();

        // ==========================================
        // Camera - try multiple methods
        // ==========================================
        pCameraInstance = FindCameraDistance();

        // ==========================================
        // Results
        // ==========================================
        LOG("=== Init Results ===");
        LOG("  EntitySystem: 0x%llX %s", pGameEntitySystem, pGameEntitySystem ? "OK" : "FAIL");
        LOG("  LocalPlayer:  0x%llX %s", pLocalPlayer, pLocalPlayer ? "OK" : "FAIL");
        LOG("  ViewMatrix:   0x%llX %s", pViewMatrix, pViewMatrix ? "OK" : "FAIL");
        LOG("  GameRules:    0x%llX %s", pGameRules, pGameRules ? "OK" : "FAIL");
        LOG("  Camera:       0x%llX %s", pCameraInstance, pCameraInstance ? "OK" : "FAIL");

        // Debug: dump some entities to verify entity system works
        if (pGameEntitySystem) {
            LOG("Max entities: %d", GetMaxEntities());
            DebugDumpEntities(200);
        }

        return pGameEntitySystem != 0;
    }

    // ============================================
    // OnFrame
    // ============================================
    void OnFrame() {
        bInGame = false;

        if (!pGameEntitySystem) return;

        auto ctrl = GetLocalPlayerController();
        if (!ctrl) return;

        auto hero = GetLocalHero();
        if (!hero) return;

        bInGame = true;

        // Update view matrix
        if (pViewMatrix) {
            // Try direct read first
            auto testMat = Memory::Read<Matrix4x4>(pViewMatrix);

            // Check if it looks valid (non-zero)
            bool valid = false;
            for (int i = 0; i < 4 && !valid; i++)
                for (int j = 0; j < 4 && !valid; j++)
                    if (testMat.m[i][j] != 0.f) valid = true;

            if (valid) {
                cachedViewMatrix = testMat;
            } else {
                // It might be a pointer to the matrix
                auto matPtr = Memory::Read<uintptr_t>(pViewMatrix);
                if (matPtr && !IsBadReadPtr((void*)matPtr, sizeof(Matrix4x4))) {
                    auto testMat2 = Memory::Read<Matrix4x4>(matPtr);
                    bool valid2 = false;
                    for (int i = 0; i < 4 && !valid2; i++)
                        for (int j = 0; j < 4 && !valid2; j++)
                            if (testMat2.m[i][j] != 0.f) valid2 = true;
                    if (valid2) cachedViewMatrix = testMat2;
                }
            }
        }
    }

    // ============================================
    // Entity access
    // ============================================
    C_BaseEntity* GetEntityByIndex(int index) {
        if (!pGameEntitySystem || index < 0) return nullptr;

        auto entityList = Memory::Read<uintptr_t>(
            pGameEntitySystem + Offsets::EntitySystem::EntityList);
        if (!entityList) return nullptr;

        int chunk = index >> 9;
        int slot = index & 0x1FF;

        auto chunkAddr = entityList + (uintptr_t)chunk * 0x8;
        if (IsBadReadPtr((void*)chunkAddr, 8)) return nullptr;
        auto chunkPtr = Memory::Read<uintptr_t>(chunkAddr);
        if (!chunkPtr) return nullptr;

        // Each entry in chunk is CEntityIdentity (0x78 bytes)
        // CEntityIdentity[0] = pointer back to entity (CEntityInstance*)
        auto identityAddr = chunkPtr + (uintptr_t)slot * 0x78;
        if (IsBadReadPtr((void*)identityAddr, 8)) return nullptr;

        auto entityPtr = Memory::Read<uintptr_t>(identityAddr);
        if (!entityPtr || entityPtr < 0x10000) return nullptr;
        if (IsBadReadPtr((void*)entityPtr, 16)) return nullptr;

        return reinterpret_cast<C_BaseEntity*>(entityPtr);
    }

    C_BaseEntity* GetEntityFromHandle(uint32_t handle) {
        if (handle == 0xFFFFFFFF || handle == 0) return nullptr;
        return GetEntityByIndex(handle & 0x7FFF);
    }

    C_DOTAPlayerController* GetLocalPlayerController() {
        if (!pLocalPlayer) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(pLocalPlayer);
        if (!ptr || ptr < 0x10000) return nullptr;
        if (IsBadReadPtr((void*)ptr, 16)) return nullptr;
        return reinterpret_cast<C_DOTAPlayerController*>(ptr);
    }

    C_DOTA_BaseNPC_Hero* GetLocalHero() {
        auto ctrl = GetLocalPlayerController();
        if (!ctrl) return nullptr;
        auto handle = ctrl->GetAssignedHeroHandle();
        if (handle == 0xFFFFFFFF || handle == 0) return nullptr;
        auto ent = GetEntityFromHandle(handle);
        if (!ent) return nullptr;
        return reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);
    }

    Matrix4x4 GetViewMatrix() { return cachedViewMatrix; }

    float GetGameTime() {
        if (!pGameRules) return 0.f;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules || IsBadReadPtr((void*)rules, 8)) return 0.f;
        return Memory::Read<float>(rules + Offsets::GameRules::GameTime);
    }

    bool IsGamePaused() {
        if (!pGameRules) return false;
        auto rules = Memory::Read<uintptr_t>(pGameRules);
        if (!rules || IsBadReadPtr((void*)rules, 8)) return false;
        return Memory::Read<bool>(rules + Offsets::GameRules::IsPaused);
    }

    bool IsInGame() { return bInGame; }

    int GetMaxEntities() {
        if (!pGameEntitySystem) return 0;
        auto val = Memory::Read<int>(pGameEntitySystem + Offsets::EntitySystem::HighestEntityIndex);
        return (val > 0 && val < 32768) ? val : 0;
    }

    // ============================================
    // Collections
    // ============================================
    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& out) {
        out.clear();
        if (!npc || !npc->IsValid()) return;
        int cnt = npc->GetAbilityCount();
        if (cnt <= 0 || cnt > 35) return;
        for (int i = 0; i < cnt; i++) {
            auto h = npc->GetAbilityHandle(i);
            if (h == 0xFFFFFFFF || h == 0) continue;
            auto e = GetEntityFromHandle(h);
            if (!e || !e->IsValid()) continue;
            auto ab = reinterpret_cast<C_DOTABaseAbility*>(e);
            AbilityInfo info;
            info.level = ab->GetLevel();
            info.cooldown = ab->GetCooldown();
            info.cooldownMax = ab->GetCooldownLength();
            info.manaCost = ab->GetManaCost();
            info.charges = ab->GetCharges();
            info.chargeRestore = ab->GetChargeRestoreTime();
            info.isHidden = ab->IsHidden();
            info.isActivated = ab->IsActivated();
            info.isToggled = ab->IsToggled();
            info.inPhase = ab->IsInAbilityPhase();
            auto cn = e->GetClassName();
            info.name = (cn && strlen(cn) > 0 && strlen(cn) < 256) ? cn : ("ability_" + std::to_string(i));
            info.isUltimate = (ab->GetMaxLevel() > 0 && ab->GetMaxLevel() <= 3 && i >= 5);
            if (info.isHidden && info.level == 0) continue;
            out.push_back(info);
        }
    }

    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& out) {
        out.clear();
        if (!npc || !npc->IsValid()) return;
        for (int s = 0; s < 16; s++) {
            auto h = npc->GetItemHandle(s);
            if (h == 0xFFFFFFFF || h == 0) continue;
            auto e = GetEntityFromHandle(h);
            if (!e || !e->IsValid()) continue;
            auto it = reinterpret_cast<C_DOTA_Item*>(e);
            ItemInfo info;
            info.slot = s;
            info.charges = it->GetCurrentCharges();
            info.cooldown = it->GetCooldown();
            info.cooldownMax = it->GetCooldownLength();
            auto cn = e->GetClassName();
            info.name = (cn && strlen(cn) > 0 && strlen(cn) < 256) ? cn : ("item_" + std::to_string(s));
            out.push_back(info);
        }
    }

    void CollectHeroes(std::vector<HeroData>& out) {
        out.clear();
        if (!bInGame) return;
        int maxEnt = GetMaxEntities();
        if (maxEnt <= 0) return;
        int limit = (std::min)(maxEnt, 512);
        for (int i = 0; i < limit; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent || !ent->IsValid()) continue;
            auto cn = ent->GetClassName();
            if (!cn || strlen(cn) < 5) continue;
            if (!strstr(cn, "Hero")) continue;
            if (!strstr(cn, "C_DOTA_Unit_Hero") && !strstr(cn, "CDOTA_Unit_Hero")) continue;

            auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(ent);
            auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);
            HeroData d;
            d.entity = hero; d.pos = hero->GetPosition(); d.team = hero->GetTeam();
            d.health = ent->GetHealth(); d.maxHP = ent->GetMaxHealth();
            d.mana = npc->GetMana(); d.maxMana = npc->GetMaxMana();
            d.hpRegen = npc->GetHealthRegen(); d.mpRegen = npc->GetManaRegen();
            d.level = npc->GetLevel(); d.heroID = hero->GetHeroID();
            d.playerID = hero->GetPlayerID(); d.alive = ent->IsAlive();
            d.illusion = npc->IsIllusion() || hero->IsReplicating();
            d.moveSpeed = npc->GetMoveSpeed(); d.attackRange = npc->GetAttackRange();
            d.primaryAttr = hero->GetPrimaryAttributeName();
            auto un = npc->GetUnitName();
            d.name = (un && strlen(un) < 256) ? un : cn;
            d.str = hero->GetStrengthTotal(); d.agi = hero->GetAgilityTotal(); d.intel = hero->GetIntellectTotal();
            d.stunned = npc->IsStunned(); d.silenced = npc->IsSilenced();
            d.hexed = npc->IsHexed(); d.rooted = npc->IsRooted();
            d.invisible = npc->IsInvisible(); d.invulnerable = npc->IsInvulnerable();
            d.magicImmune = npc->IsMagicImmune(); d.disarmed = npc->IsDisarmed(); d.muted = npc->IsMuted();
            CollectAbilities(npc, d.abilities);
            CollectItems(npc, d.items);
            out.push_back(std::move(d));
        }
    }
}
