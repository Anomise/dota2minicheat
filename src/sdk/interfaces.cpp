#include "interfaces.h"
#include "../utils/memory.h"
#include "../utils/pattern_scan.h"
#include <cstring>
#include <algorithm>

#ifdef _DEBUG
#include <cstdio>
#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

namespace SDK {

    // ============================================
    // Source 2 Interface Registry
    // ============================================
    struct InterfaceReg {
        void* (*createFn)();
        const char* name;
        InterfaceReg* next;
    };

    uintptr_t FindInterface(const char* moduleName, const char* interfaceName) {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (!hMod) {
            LOG("[SDK] Module not found: %s", moduleName);
            return 0;
        }

        // Get CreateInterface export
        auto createInterface = reinterpret_cast<uintptr_t>(GetProcAddress(hMod, "CreateInterface"));
        if (!createInterface) {
            LOG("[SDK] CreateInterface not found in %s", moduleName);
            return 0;
        }

        // Follow the function to find the interface registry list
        // CreateInterface typically does: mov rax, [registry_ptr]; ...
        // We scan for the interface name in the registry linked list

        // Alternative: call CreateInterface directly
        typedef void* (*CreateInterfaceFn)(const char*, int*);
        auto fn = reinterpret_cast<CreateInterfaceFn>(createInterface);

        int returnCode = 0;
        void* iface = fn(interfaceName, &returnCode);

        if (iface) {
            LOG("[SDK] Found interface %s @ 0x%p", interfaceName, iface);
            return reinterpret_cast<uintptr_t>(iface);
        }

        LOG("[SDK] Interface %s not found via CreateInterface", interfaceName);
        return 0;
    }

    // ============================================
    // Pattern-based fallback finders
    // ============================================

    static uintptr_t FindGameEntitySystem() {
        // Method 1: Pattern scan
        auto pat = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8B 1D ?? ?? ?? ?? 48 85 DB 74 ?? 48 8B CB");
        if (pat) {
            auto result = Memory::ResolveRelative(pat, 3, 7);
            auto ptr = Memory::Read<uintptr_t>(result);
            if (ptr) {
                LOG("[SDK] GES via pattern: 0x%llX", ptr);
                return ptr;
            }
        }

        // Method 2: Interface registry
        auto iface = FindInterface(Offsets::CLIENT_DLL, "GameResourceServiceClientV001");
        if (iface) {
            // GameResourceService -> GetGameEntitySystem() is typically at vtable index 1
            auto vtable = Memory::Read<uintptr_t>(iface);
            if (vtable) {
                auto getGES = Memory::Read<uintptr_t>(vtable + 1 * 8); // second vfunc
                if (getGES) {
                    // Call it or parse the function
                    // The function typically just returns a global pointer
                    // Try reading relative address from the function body
                    // mov rax, [rip+offset]; ret
                    auto funcBody = getGES;
                    auto byte1 = Memory::Read<uint8_t>(funcBody);
                    if (byte1 == 0x48) { // REX prefix
                        auto byte2 = Memory::Read<uint8_t>(funcBody + 1);
                        auto byte3 = Memory::Read<uint8_t>(funcBody + 2);
                        if (byte2 == 0x8B && (byte3 == 0x05 || byte3 == 0x0D || byte3 == 0x15 || byte3 == 0x1D || byte3 == 0x25 || byte3 == 0x2D || byte3 == 0x35 || byte3 == 0x3D)) {
                            auto resolved = Memory::ResolveRelative(funcBody, 3, 7);
                            auto ptr = Memory::Read<uintptr_t>(resolved);
                            if (ptr) {
                                LOG("[SDK] GES via interface vtable: 0x%llX", ptr);
                                return ptr;
                            }
                        }
                    }
                }
            }
        }

        // Method 3: Scan for entity list pattern
        auto pat2 = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 48 85 C0");
        if (pat2) {
            auto result = Memory::ResolveRelative(pat2, 3, 7);
            auto ptr = Memory::Read<uintptr_t>(result);
            if (ptr) {
                LOG("[SDK] GES via pattern2: 0x%llX", ptr);
                return ptr;
            }
        }

        LOG("[SDK] GES not found!");
        return 0;
    }

    static uintptr_t FindLocalPlayerController() {
        // Pattern: 48 89 05 ?? ?? ?? ?? 48 8B 4E
        auto pat = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 89 05 ?? ?? ?? ?? 48 8B 4E");
        if (pat) {
            auto result = Memory::ResolveRelative(pat, 3, 7);
            LOG("[SDK] LocalPlayer via pattern: 0x%llX", result);
            return result;
        }

        // Alt pattern
        auto pat2 = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 83 3D ?? ?? ?? ?? 00 74 ?? 48 8B 0D");
        if (pat2) {
            auto result = Memory::ResolveRelative(pat2, 3, 8);
            LOG("[SDK] LocalPlayer via pattern2: 0x%llX", result);
            return result;
        }

        LOG("[SDK] LocalPlayer not found!");
        return 0;
    }

    static uintptr_t FindViewMatrix() {
        auto pat = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8D 05 ?? ?? ?? ?? 48 C7 05");
        if (pat) {
            auto result = Memory::ResolveRelative(pat, 3, 7);
            LOG("[SDK] ViewMatrix via pattern: 0x%llX", result);
            return result;
        }

        // Alternative patterns for view/projection matrix
        auto pat2 = Memory::FindPattern(Offsets::ENGINE_DLL,
            "48 8D 0D ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00 E8");
        if (pat2) {
            auto result = Memory::ResolveRelative(pat2, 3, 7);
            LOG("[SDK] ViewMatrix via engine pattern: 0x%llX", result);
            return result;
        }

        LOG("[SDK] ViewMatrix not found!");
        return 0;
    }

    static uintptr_t FindGameRules() {
        auto pat = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 48");
        if (pat) {
            auto result = Memory::ResolveRelative(pat, 3, 7);
            LOG("[SDK] GameRules via pattern: 0x%llX", result);
            return result;
        }

        // Alternative
        auto pat2 = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 89 1D ?? ?? ?? ?? 48 89 5C 24 ?? E8");
        if (pat2) {
            auto result = Memory::ResolveRelative(pat2, 3, 7);
            LOG("[SDK] GameRules via pattern2: 0x%llX", result);
            return result;
        }

        LOG("[SDK] GameRules not found!");
        return 0;
    }

    static uintptr_t FindCamera() {
        auto pat = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8D 05 ?? ?? ?? ?? 48 89 01 F3 0F 10 05");
        if (pat) {
            auto result = Memory::ResolveRelative(pat, 3, 7);
            LOG("[SDK] Camera via pattern: 0x%llX", result);
            return result;
        }

        // Alternative: search for CDOTA_Camera vtable
        auto pat2 = Memory::FindPattern(Offsets::CLIENT_DLL,
            "48 8D 05 ?? ?? ?? ?? 48 89 01 F3 0F 10");
        if (pat2) {
            auto result = Memory::ResolveRelative(pat2, 3, 7);
            LOG("[SDK] Camera via pattern2: 0x%llX", result);
            return result;
        }

        LOG("[SDK] Camera not found!");
        return 0;
    }

    // ============================================
    // Initialize
    // ============================================
    bool Initialize() {
        LOG("[SDK] Initializing...");

        // Wait for client.dll
        int waitCount = 0;
        while (!Memory::GetModuleBase(Offsets::CLIENT_DLL)) {
            Sleep(1000);
            if (++waitCount > 120) { // 2 min timeout
                LOG("[SDK] Timeout waiting for client.dll");
                return false;
            }
        }

        LOG("[SDK] client.dll loaded at 0x%llX (size: 0x%llX)",
            Memory::GetModuleBase(Offsets::CLIENT_DLL),
            Memory::GetModuleSize(Offsets::CLIENT_DLL));

        // Wait for engine
        waitCount = 0;
        while (!Memory::GetModuleBase(Offsets::ENGINE_DLL)) {
            Sleep(1000);
            if (++waitCount > 60) {
                LOG("[SDK] Timeout waiting for engine2.dll");
                return false;
            }
        }

        // Wait additional time for game to fully initialize
        Sleep(10000);

        LOG("[SDK] Starting pointer resolution...");

        // Find all pointers
        pGameEntitySystem = FindGameEntitySystem();
        pLocalPlayer      = FindLocalPlayerController();
        pViewMatrix       = FindViewMatrix();
        pGameRules        = FindGameRules();
        pCameraInstance   = FindCamera();

        LOG("[SDK] Results:");
        LOG("[SDK]   GameEntitySystem: 0x%llX %s", pGameEntitySystem, pGameEntitySystem ? "OK" : "FAIL");
        LOG("[SDK]   LocalPlayer:      0x%llX %s", pLocalPlayer, pLocalPlayer ? "OK" : "FAIL");
        LOG("[SDK]   ViewMatrix:       0x%llX %s", pViewMatrix, pViewMatrix ? "OK" : "FAIL");
        LOG("[SDK]   GameRules:        0x%llX %s", pGameRules, pGameRules ? "OK" : "FAIL");
        LOG("[SDK]   Camera:           0x%llX %s", pCameraInstance, pCameraInstance ? "OK" : "FAIL");

        // At minimum we need entity system
        bool success = pGameEntitySystem != 0;
        LOG("[SDK] Init %s", success ? "SUCCESS" : "PARTIAL (entity system not found)");
        return success;
    }

    // ============================================
    // OnFrame - update cached data
    // ============================================
    void OnFrame() {
        // Check if we're in a game
        bInGame = false;

        auto localCtrl = GetLocalPlayerController();
        if (!localCtrl) return;

        auto localHero = GetLocalHero();
        if (!localHero) return;

        bInGame = true;

        // Cache view matrix
        if (pViewMatrix) {
            cachedViewMatrix = Memory::Read<Matrix4x4>(pViewMatrix);
        }
    }

    // ============================================
    // Accessors
    // ============================================

    C_DOTAPlayerController* GetLocalPlayerController() {
        if (!pLocalPlayer) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(pLocalPlayer);
        if (!ptr || ptr < 0x10000) return nullptr;
        if (IsBadReadPtr(reinterpret_cast<void*>(ptr), 8)) return nullptr;
        return reinterpret_cast<C_DOTAPlayerController*>(ptr);
    }

    C_DOTA_BaseNPC_Hero* GetLocalHero() {
        auto ctrl = GetLocalPlayerController();
        if (!ctrl) return nullptr;

        auto heroHandle = ctrl->GetAssignedHeroHandle();
        if (heroHandle == 0xFFFFFFFF || heroHandle == 0) return nullptr;

        auto heroEnt = GetEntityFromHandle(heroHandle);
        if (!heroEnt) return nullptr;

        // Verify it's alive or at least valid
        if (IsBadReadPtr(reinterpret_cast<void*>(heroEnt), 8)) return nullptr;

        return reinterpret_cast<C_DOTA_BaseNPC_Hero*>(heroEnt);
    }

    Matrix4x4 GetViewMatrix() {
        return cachedViewMatrix;
    }

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

    bool IsInGame() {
        return bInGame;
    }

    int GetMaxEntities() {
        if (!pGameEntitySystem) return 0;
        auto val = Memory::Read<int>(pGameEntitySystem + Offsets::EntitySystem::HighestEntityIndex);
        if (val <= 0 || val > 32768) return 0;
        return val;
    }

    // ============================================
    // Entity access
    // ============================================

    C_BaseEntity* GetEntityByIndex(int index) {
        if (!pGameEntitySystem || index < 0) return nullptr;

        auto entityList = Memory::Read<uintptr_t>(pGameEntitySystem + Offsets::EntitySystem::EntityList);
        if (!entityList) return nullptr;

        // Source 2 chunked entity list
        int chunk = index >> 9;
        int slot = index & 0x1FF;

        auto chunkPtr = Memory::Read<uintptr_t>(entityList + static_cast<uintptr_t>(chunk) * 0x8);
        if (!chunkPtr) return nullptr;

        // Each slot in chunk is 0x78 bytes (CEntityIdentity)
        auto identityAddr = chunkPtr + static_cast<uintptr_t>(slot) * 0x78;
        if (IsBadReadPtr(reinterpret_cast<void*>(identityAddr), 8)) return nullptr;

        // CEntityIdentity -> entity pointer is at offset 0 typically
        // But identityAddr IS the identity, and identity has entity ptr
        // In Source 2, the handle resolves to CEntityInstance directly
        // The chunk stores CEntityIdentity structs
        // CEntityIdentity at +0x0 doesn't point to entity - it IS part of entity

        // Actually, the chunk entry at slot * 0x78 gives us the CEntityIdentity
        // which is embedded in the entity at entity + 0x10
        // So entity = identityAddr - 0x10... no.

        // Let's try: the chunk stores pointers to entities directly
        // Some implementations: chunkPtr[slot] = entity pointer

        // Try method: read pointer from chunk
        auto entityPtr = Memory::Read<uintptr_t>(identityAddr);
        if (!entityPtr || entityPtr < 0x10000) return nullptr;
        if (IsBadReadPtr(reinterpret_cast<void*>(entityPtr), 16)) return nullptr;

        return reinterpret_cast<C_BaseEntity*>(entityPtr);
    }

    C_BaseEntity* GetEntityFromHandle(uint32_t handle) {
        if (handle == 0xFFFFFFFF || handle == 0) return nullptr;
        int index = handle & 0x7FFF;
        return GetEntityByIndex(index);
    }

    // ============================================
    // Safe string reader
    // ============================================
    static bool SafeReadString(uintptr_t addr, char* buf, size_t maxLen) {
        if (!addr || IsBadReadPtr(reinterpret_cast<void*>(addr), 1)) return false;
        for (size_t i = 0; i < maxLen - 1; i++) {
            if (IsBadReadPtr(reinterpret_cast<void*>(addr + i), 1)) {
                buf[i] = 0;
                return i > 0;
            }
            buf[i] = Memory::Read<char>(addr + i);
            if (buf[i] == 0) return i > 0;
        }
        buf[maxLen - 1] = 0;
        return true;
    }

    // ============================================
    // Ability collection
    // ============================================
    void CollectAbilities(C_DOTA_BaseNPC* npc, std::vector<AbilityInfo>& out) {
        out.clear();
        if (!npc) return;

        int cnt = npc->GetAbilityCount();
        if (cnt <= 0 || cnt > 35) return;

        for (int i = 0; i < cnt; i++) {
            auto handle = npc->GetAbilityHandle(i);
            if (handle == 0xFFFFFFFF || handle == 0) continue;

            auto ent = GetEntityFromHandle(handle);
            if (!ent) continue;
            if (IsBadReadPtr(reinterpret_cast<void*>(ent), 0x700)) continue;

            auto ability = reinterpret_cast<C_DOTABaseAbility*>(ent);

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

            // Sanity checks
            if (info.cooldown < 0 || info.cooldown > 9999) info.cooldown = 0;
            if (info.cooldownMax < 0 || info.cooldownMax > 9999) info.cooldownMax = 0;
            if (info.manaCost < 0 || info.manaCost > 9999) info.manaCost = 0;
            if (info.level < 0 || info.level > 30) info.level = 0;

            auto className = ent->GetClassName();
            if (className && strlen(className) > 0 && strlen(className) < 256)
                info.name = className;
            else
                info.name = "ability_" + std::to_string(i);

            info.isUltimate = (ability->GetMaxLevel() > 0 && ability->GetMaxLevel() <= 3 && i >= 5);

            if (info.isHidden && info.level == 0) continue;

            out.push_back(info);
        }
    }

    // ============================================
    // Item collection
    // ============================================
    void CollectItems(C_DOTA_BaseNPC* npc, std::vector<ItemInfo>& out) {
        out.clear();
        if (!npc) return;

        for (int slot = 0; slot < 16; slot++) {
            auto handle = npc->GetItemHandle(slot);
            if (handle == 0xFFFFFFFF || handle == 0) continue;

            auto ent = GetEntityFromHandle(handle);
            if (!ent) continue;
            if (IsBadReadPtr(reinterpret_cast<void*>(ent), 0x700)) continue;

            auto item = reinterpret_cast<C_DOTA_Item*>(ent);

            ItemInfo info;
            info.slot = slot;
            info.charges = item->GetCurrentCharges();
            info.cooldown = item->GetCooldown();
            info.cooldownMax = item->GetCooldownLength();

            // Sanity
            if (info.charges < 0 || info.charges > 999) info.charges = 0;
            if (info.cooldown < 0 || info.cooldown > 9999) info.cooldown = 0;

            auto className = ent->GetClassName();
            if (className && strlen(className) > 0 && strlen(className) < 256)
                info.name = className;
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

        // Heroes are typically in the first ~200 entities
        int scanLimit = (std::min)(maxEnt, 512);

        for (int i = 0; i < scanLimit; i++) {
            auto ent = GetEntityByIndex(i);
            if (!ent) continue;

            // Quick alive check first (cheap)
            auto lifeState = ent->GetLifeState();
            auto hp = ent->GetHealth();

            // Get class name
            auto className = ent->GetClassName();
            if (!className || strlen(className) < 10) continue;

            // Check if hero
            if (strstr(className, "Hero") == nullptr) continue;
            if (strstr(className, "CDOTA_Unit_Hero") == nullptr &&
                strstr(className, "C_DOTA_Unit_Hero") == nullptr) continue;

            auto npc = reinterpret_cast<C_DOTA_BaseNPC*>(ent);
            auto hero = reinterpret_cast<C_DOTA_BaseNPC_Hero*>(ent);

            HeroData data;
            data.entity = hero;
            data.pos = hero->GetPosition();
            data.team = hero->GetTeam();
            data.health = hp;
            data.maxHP = hero->GetMaxHealth();
            data.mana = npc->GetMana();
            data.maxMana = npc->GetMaxMana();
            data.hpRegen = npc->GetHealthRegen();
            data.mpRegen = npc->GetManaRegen();
            data.level = npc->GetLevel();
            data.heroID = hero->GetHeroID();
            data.playerID = hero->GetPlayerID();
            data.alive = (lifeState == 0 && hp > 0);
            data.illusion = npc->IsIllusion() || hero->IsReplicating();
            data.moveSpeed = npc->GetMoveSpeed();
            data.attackRange = npc->GetAttackRange();
            data.primaryAttr = hero->GetPrimaryAttributeName();

            // Safe unit name
            auto unitName = npc->GetUnitName();
            if (unitName && strlen(unitName) < 256)
                data.name = unitName;
            else
                data.name = className;

            // Stats (with sanity checks)
            data.str = hero->GetStrengthTotal();
            data.agi = hero->GetAgilityTotal();
            data.intel = hero->GetIntellectTotal();
            if (data.str < 0 || data.str > 9999) data.str = 0;
            if (data.agi < 0 || data.agi > 9999) data.agi = 0;
            if (data.intel < 0 || data.intel > 9999) data.intel = 0;

            // Status
            data.stunned = npc->IsStunned();
            data.silenced = npc->IsSilenced();
            data.hexed = npc->IsHexed();
            data.rooted = npc->IsRooted();
            data.invisible = npc->IsInvisible();
            data.invulnerable = npc->IsInvulnerable();
            data.magicImmune = npc->IsMagicImmune();
            data.disarmed = npc->IsDisarmed();
            data.muted = npc->IsMuted();

            // Collect abilities and items
            CollectAbilities(npc, data.abilities);
            CollectItems(npc, data.items);

            out.push_back(std::move(data));
        }
    }
}
