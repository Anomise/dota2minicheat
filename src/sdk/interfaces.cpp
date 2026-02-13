#include "interfaces.h"
#include "../utils/memory.h"
#include "../utils/logger.h"
#include <cstring>
#include <algorithm>

namespace SDK {

    void* GetInterface(const char* moduleName, const char* ifaceName) {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (!hMod) {
            LOG_ERROR("Module not loaded: %s", moduleName);
            return nullptr;
        }
        using CreateInterfaceFn = void*(*)(const char*, int*);
        auto fn = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hMod, "CreateInterface"));
        if (!fn) {
            LOG_ERROR("CreateInterface not found in %s", moduleName);
            return nullptr;
        }
        int ret = 0;
        void* iface = fn(ifaceName, &ret);
        if (iface)
            LOG_SUCCESS("Interface %s::%s = %p", moduleName, ifaceName, iface);
        else
            LOG_ERROR("Interface %s::%s NOT FOUND", moduleName, ifaceName);
        return iface;
    }

    static void* CallVFunc(void* iface, int index) {
        if (!iface) return nullptr;
        auto vtable = *reinterpret_cast<void***>(iface);
        if (!vtable) return nullptr;
        using Fn = void*(*)(void*);
        return reinterpret_cast<Fn>(vtable[index])(iface);
    }

    static uintptr_t FindCameraDistance() {
        LOG_INFO("Searching for camera...");

        auto pat = Memory::FindPattern("client.dll",
            "F3 0F 10 05 ?? ?? ?? ?? F3 0F 5C C1 F3 0F 58 05");
        if (pat) {
            auto addr = Memory::ResolveRelative(pat, 4, 8);
            LOG_SUCCESS("Camera distance (convar pattern): 0x%llX", addr);
            return addr;
        }
        LOG_WARN("Camera pattern 1 not found");

        auto pat2 = Memory::FindPattern("client.dll",
            "F3 0F 10 ?? ?? ?? ?? ?? 0F 2F ?? F3 0F 10");
        if (pat2) {
            auto addr = Memory::ResolveRelative(pat2, 4, 8);
            LOG_SUCCESS("Camera distance (pattern2): 0x%llX", addr);
            return addr;
        }
        LOG_WARN("Camera pattern 2 not found");

        LOG_ERROR("Camera distance not found - will use PlayerController zoom");
        return 0;
    }

    static uintptr_t FindViewMatrix() {
        LOG_INFO("Searching for ViewMatrix...");

        auto pat = Memory::FindPattern("client.dll",
            "48 8D 05 ?? ?? ?? ?? 48 C7 05");
        if (pat) {
            auto addr = Memory::ResolveRelative(pat, 3, 7);
            LOG_SUCCESS("ViewMatrix (pattern1): 0x%llX", addr);
            return addr;
        }
        LOG_WARN("ViewMatrix pattern 1 not found");

        auto pat2 = Memory::FindPattern("client.dll",
            "48 89 05 ?? ?? ?? ?? 48 8B C8 48 85 C0");
        if (pat2) {
            auto addr = Memory::ResolveRelative(pat2, 3, 7);
            LOG_SUCCESS("ViewMatrix (ViewRender): 0x%llX", addr);
            return addr;
        }
        LOG_WARN("ViewMatrix pattern 2 not found");

        auto pat3 = Memory::FindPattern("engine2.dll",
            "48 8D 0D ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00");
        if (pat3) {
            auto addr = Memory::ResolveRelative(pat3, 3, 7);
            LOG_SUCCESS("ViewMatrix (engine): 0x%llX", addr);
            return addr;
        }
        LOG_WARN("ViewMatrix pattern 3 not found");

        LOG_ERROR("ViewMatrix not found - ESP/Awareness will not work");
        return 0;
    }

    static void DebugDumpEntities(int count) {
        LOG_INFO("--- Entity Dump (first %d) ---", count);
        int found = 0;
        for (int i = 0; i < count && found < 30; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent) continue;
            auto cn = ent->GetClassName();
            auto en = ent->GetEntityName();
            if (cn && strlen(cn) > 0) {
                int hp = ent->GetHealth();
                int team = ent->GetTeam();
                auto pos = ent->GetPosition();
                LOG_INFO("  [%d] class='%s' name='%s' hp=%d team=%d pos=(%.0f,%.0f,%.0f)",
                    i, cn, en ? en : "?", hp, team, pos.x, pos.y, pos.z);
                found++;
            }
        }
        LOG_INFO("--- End dump (%d entities found) ---", found);
    }

    bool Initialize() {
        LOG_INFO("=== SDK Initialization ===");

        // Wait for client.dll
        int waitCount = 0;
        LOG_INFO("Waiting for client.dll...");
        while (!(clientBase = Memory::GetModuleBase("client.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) { LOG_ERROR("TIMEOUT: client.dll not found after 120s"); return false; }
            if (waitCount % 10 == 0) LOG_INFO("Still waiting for client.dll... (%ds)", waitCount);
        }
        clientSize = Memory::GetModuleSize("client.dll");
        LOG_SUCCESS("client.dll: 0x%llX (size: 0x%llX = %.1f MB)",
            clientBase, clientSize, clientSize / 1048576.0);

        LOG_INFO("Waiting for engine2.dll...");
        while (!(engineBase = Memory::GetModuleBase("engine2.dll"))) {
            Sleep(1000);
            if (++waitCount > 120) { LOG_ERROR("TIMEOUT: engine2.dll not found"); return false; }
        }
        engineSize = Memory::GetModuleSize("engine2.dll");
        LOG_SUCCESS("engine2.dll: 0x%llX (size: 0x%llX)", engineBase, engineSize);

        LOG_INFO("Waiting 8s for game to fully initialize...");
        Sleep(8000);

        // ==========================================
        // GameEntitySystem
        // ==========================================
        LOG_INFO("--- Finding GameEntitySystem ---");
        auto pGameResourceService = GetInterface("engine2.dll", "GameResourceServiceClientV001");
        if (pGameResourceService) {
            auto gesPtr = CallVFunc(pGameResourceService, 1);
            if (gesPtr) {
                pGameEntitySystem = reinterpret_cast<uintptr_t>(gesPtr);
                LOG_SUCCESS("GameEntitySystem (via interface): 0x%llX", pGameEntitySystem);
            } else {
                LOG_WARN("CallVFunc(GameResourceService, 1) returned null");
            }
        }

        if (!pGameEntitySystem) {
            LOG_INFO("Trying pattern scan for GameEntitySystem...");
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 1D ?? ?? ?? ?? 48 85 DB 74");
            if (pat) {
                auto addr = Memory::ResolveRelative(pat, 3, 7);
                pGameEntitySystem = Memory::Read<uintptr_t>(addr);
                if (pGameEntitySystem)
                    LOG_SUCCESS("GameEntitySystem (pattern): 0x%llX", pGameEntitySystem);
                else
                    LOG_ERROR("GameEntitySystem pattern found but pointer is null");
            } else {
                LOG_ERROR("GameEntitySystem pattern not found");
            }
        }

        // ==========================================
        // Source2Client
        // ==========================================
        LOG_INFO("--- Finding Source2Client ---");
        auto pSource2Client = GetInterface("client.dll", "Source2Client002");

        // ==========================================
        // LocalPlayer
        // ==========================================
        LOG_INFO("--- Finding LocalPlayer ---");
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 89 05 ?? ?? ?? ?? 48 8B 4E");
            if (pat) {
                pLocalPlayer = Memory::ResolveRelative(pat, 3, 7);
                LOG_SUCCESS("LocalPlayer (pattern1): 0x%llX", pLocalPlayer);
            } else {
                LOG_WARN("LocalPlayer pattern 1 not found");
            }
        }
        if (!pLocalPlayer) {
            auto pat2 = Memory::FindPattern("client.dll",
                "48 89 3D ?? ?? ?? ?? 48 8B 5C 24");
            if (pat2) {
                pLocalPlayer = Memory::ResolveRelative(pat2, 3, 7);
                LOG_SUCCESS("LocalPlayer (pattern2): 0x%llX", pLocalPlayer);
            } else {
                LOG_WARN("LocalPlayer pattern 2 not found");
            }
        }
        if (!pLocalPlayer) {
            auto pat3 = Memory::FindPattern("client.dll",
                "48 89 1D ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3");
            if (pat3) {
                pLocalPlayer = Memory::ResolveRelative(pat3, 3, 7);
                LOG_SUCCESS("LocalPlayer (pattern3): 0x%llX", pLocalPlayer);
            } else {
                LOG_ERROR("LocalPlayer not found with any pattern");
            }
        }

        // ==========================================
        // GameRules
        // ==========================================
        LOG_INFO("--- Finding GameRules ---");
        {
            auto pat = Memory::FindPattern("client.dll",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 48");
            if (pat) {
                pGameRules = Memory::ResolveRelative(pat, 3, 7);
                LOG_SUCCESS("GameRules (pattern1): 0x%llX", pGameRules);
            } else {
                LOG_WARN("GameRules pattern 1 not found");
            }
        }
        if (!pGameRules) {
            auto pat2 = Memory::FindPattern("client.dll",
                "48 89 0D ?? ?? ?? ?? 48 89 41");
            if (pat2) {
                pGameRules = Memory::ResolveRelative(pat2, 3, 7);
                LOG_SUCCESS("GameRules (pattern2): 0x%llX", pGameRules);
            } else {
                LOG_ERROR("GameRules not found with any pattern");
            }
        }

        // ==========================================
        // ViewMatrix
        // ==========================================
        LOG_INFO("--- Finding ViewMatrix ---");
        pViewMatrix = FindViewMatrix();

        // ==========================================
        // Camera
        // ==========================================
        LOG_INFO("--- Finding Camera ---");
        pCameraInstance = FindCameraDistance();

        // ==========================================
        // Summary
        // ==========================================
        LOG_INFO("==========================================");
        LOG_INFO("         SDK INIT SUMMARY");
        LOG_INFO("==========================================");

        auto StatusLog = [](const char* name, uintptr_t ptr) {
            if (ptr)
                LOG_SUCCESS("  %-20s 0x%llX", name, ptr);
            else
                LOG_ERROR("  %-20s NOT FOUND", name);
        };

        StatusLog("GameEntitySystem", pGameEntitySystem);
        StatusLog("LocalPlayer", pLocalPlayer);
        StatusLog("ViewMatrix", pViewMatrix);
        StatusLog("GameRules", pGameRules);
        StatusLog("Camera", pCameraInstance);
        LOG_INFO("==========================================");

        // Test entity system
        if (pGameEntitySystem) {
            int maxEnt = GetMaxEntities();
            LOG_INFO("Max entities: %d", maxEnt);
            if (maxEnt > 0) {
                DebugDumpEntities(300);
            } else {
                LOG_WARN("Max entities is 0 - might not be in a game yet");
            }
        }

        // Test local player
        auto ctrl = GetLocalPlayerController();
        if (ctrl) {
            LOG_SUCCESS("LocalPlayerController found! PlayerID=%d HeroHandle=0x%X CameraZoom=%.1f",
                ctrl->GetPlayerID(), ctrl->GetAssignedHeroHandle(), ctrl->GetCameraZoom());

            auto hero = GetLocalHero();
            if (hero) {
                LOG_SUCCESS("Local hero found! Name='%s' HP=%d/%d Level=%d Team=%d",
                    hero->GetUnitName(), hero->GetHealth(), hero->GetMaxHealth(),
                    hero->GetLevel(), hero->GetTeam());
                LOG_INFO("  STR=%.1f AGI=%.1f INT=%.1f MS=%d AR=%d",
                    hero->GetStrengthTotal(), hero->GetAgilityTotal(), hero->GetIntellectTotal(),
                    hero->GetMoveSpeed(), hero->GetAttackRange());
            } else {
                LOG_WARN("Local hero not found via handle - might not have picked yet");
            }
        } else {
            LOG_WARN("LocalPlayerController not available - not in a game?");
        }

        // Test ViewMatrix
        if (pViewMatrix) {
            auto testMat = Memory::Read<Matrix4x4>(pViewMatrix);
            bool valid = false;
            for (int i = 0; i < 4 && !valid; i++)
                for (int j = 0; j < 4 && !valid; j++)
                    if (testMat.m[i][j] != 0.f) valid = true;

            if (valid) {
                LOG_SUCCESS("ViewMatrix looks valid (non-zero values found)");
                LOG_INFO("  [0][0]=%.4f [1][1]=%.4f [2][2]=%.4f [3][3]=%.4f",
                    testMat.m[0][0], testMat.m[1][1], testMat.m[2][2], testMat.m[3][3]);
            } else {
                LOG_WARN("ViewMatrix is all zeros - trying as pointer...");
                auto matPtr = Memory::Read<uintptr_t>(pViewMatrix);
                if (matPtr && !IsBadReadPtr((void*)matPtr, 64)) {
                    auto testMat2 = Memory::Read<Matrix4x4>(matPtr);
                    bool valid2 = false;
                    for (int i = 0; i < 4 && !valid2; i++)
                        for (int j = 0; j < 4 && !valid2; j++)
                            if (testMat2.m[i][j] != 0.f) valid2 = true;
                    if (valid2) {
                        LOG_SUCCESS("ViewMatrix valid as pointer! Dereferenced at 0x%llX", matPtr);
                        pViewMatrix = matPtr; // Update to direct address
                    } else {
                        LOG_ERROR("ViewMatrix still invalid after dereference");
                    }
                } else {
                    LOG_ERROR("ViewMatrix pointer dereference failed");
                }
            }
        }

        bool success = pGameEntitySystem != 0;
        if (success)
            LOG_SUCCESS("SDK initialization completed successfully!");
        else
            LOG_ERROR("SDK initialization FAILED - entity system not found");

        return success;
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

        if (pViewMatrix) {
            auto testMat = Memory::Read<Matrix4x4>(pViewMatrix);
            bool valid = false;
            for (int i = 0; i < 4 && !valid; i++)
                for (int j = 0; j < 4 && !valid; j++)
                    if (testMat.m[i][j] != 0.f) valid = true;
            if (valid) {
                cachedViewMatrix = testMat;
            } else {
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
    // Entity access (same as before)
    // ============================================
    C_BaseEntity* GetEntityByIndex(int index) {
        if (!pGameEntitySystem || index < 0) return nullptr;
        auto entityList = Memory::Read<uintptr_t>(pGameEntitySystem + Offsets::EntitySystem::EntityList);
        if (!entityList) return nullptr;
        int chunk = index >> 9;
        int slot = index & 0x1FF;
        auto chunkAddr = entityList + (uintptr_t)chunk * 0x8;
        if (IsBadReadPtr((void*)chunkAddr, 8)) return nullptr;
        auto chunkPtr = Memory::Read<uintptr_t>(chunkAddr);
        if (!chunkPtr) return nullptr;
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
        auto r = Memory::Read<uintptr_t>(pGameRules);
        if (!r || IsBadReadPtr((void*)r, 8)) return 0.f;
        return Memory::Read<float>(r + Offsets::GameRules::GameTime);
    }
    bool IsGamePaused() {
        if (!pGameRules) return false;
        auto r = Memory::Read<uintptr_t>(pGameRules);
        if (!r || IsBadReadPtr((void*)r, 8)) return false;
        return Memory::Read<bool>(r + Offsets::GameRules::IsPaused);
    }
    bool IsInGame() { return bInGame; }
    int GetMaxEntities() {
        if (!pGameEntitySystem) return 0;
        auto v = Memory::Read<int>(pGameEntitySystem + Offsets::EntitySystem::HighestEntityIndex);
        return (v > 0 && v < 32768) ? v : 0;
    }

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
            info.level = ab->GetLevel(); info.cooldown = ab->GetCooldown();
            info.cooldownMax = ab->GetCooldownLength(); info.manaCost = ab->GetManaCost();
            info.charges = ab->GetCharges(); info.chargeRestore = ab->GetChargeRestoreTime();
            info.isHidden = ab->IsHidden(); info.isActivated = ab->IsActivated();
            info.isToggled = ab->IsToggled(); info.inPhase = ab->IsInAbilityPhase();
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
            info.slot = s; info.charges = it->GetCurrentCharges();
            info.cooldown = it->GetCooldown(); info.cooldownMax = it->GetCooldownLength();
            auto cn = e->GetClassName();
            info.name = (cn && strlen(cn) > 0 && strlen(cn) < 256) ? cn : ("item_" + std::to_string(s));
            out.push_back(info);
        }
    }

    void CollectHeroes(std::vector<HeroData>& out) {
        out.clear();
        if (!bInGame) return;
        int mx = GetMaxEntities();
        if (mx <= 0) return;
        int limit = (std::min)(mx, 512);
        for (int i = 0; i < limit; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent || !ent->IsValid()) continue;
            auto cn = ent->GetClassName();
            if (!cn || strlen(cn) < 5 || !strstr(cn, "Hero")) continue;
            if (!strstr(cn, "C_DOTA_Unit_Hero") && !strstr(cn, "CDOTA_Unit_Hero")) continue;
            auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(ent);
            auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);
            HeroData d;
            d.entity=hero; d.pos=hero->GetPosition(); d.team=hero->GetTeam();
            d.health=ent->GetHealth(); d.maxHP=ent->GetMaxHealth();
            d.mana=npc->GetMana(); d.maxMana=npc->GetMaxMana();
            d.hpRegen=npc->GetHealthRegen(); d.mpRegen=npc->GetManaRegen();
            d.level=npc->GetLevel(); d.heroID=hero->GetHeroID();
            d.playerID=hero->GetPlayerID(); d.alive=ent->IsAlive();
            d.illusion=npc->IsIllusion()||hero->IsReplicating();
            d.moveSpeed=npc->GetMoveSpeed(); d.attackRange=npc->GetAttackRange();
            d.primaryAttr=hero->GetPrimaryAttributeName();
            auto un=npc->GetUnitName();
            d.name=(un&&strlen(un)<256)?un:cn;
            d.str=hero->GetStrengthTotal(); d.agi=hero->GetAgilityTotal(); d.intel=hero->GetIntellectTotal();
            d.stunned=npc->IsStunned(); d.silenced=npc->IsSilenced();
            d.hexed=npc->IsHexed(); d.rooted=npc->IsRooted();
            d.invisible=npc->IsInvisible(); d.invulnerable=npc->IsInvulnerable();
            d.magicImmune=npc->IsMagicImmune(); d.disarmed=npc->IsDisarmed(); d.muted=npc->IsMuted();
            CollectAbilities(npc, d.abilities);
            CollectItems(npc, d.items);
            out.push_back(std::move(d));
        }
    }
}
