#pragma once
#include "../utils/math.h"
#include "../utils/memory.h"
#include "offsets.h"
#include <string>
#include <vector>
#include <cstring>

namespace UnitState {
    constexpr uint64_t Stunned      = (1ULL << 2);
    constexpr uint64_t Silenced     = (1ULL << 3);
    constexpr uint64_t Muted        = (1ULL << 4);
    constexpr uint64_t Rooted       = (1ULL << 5);
    constexpr uint64_t Hexed        = (1ULL << 6);
    constexpr uint64_t Invisible    = (1ULL << 8);
    constexpr uint64_t Invulnerable = (1ULL << 10);
    constexpr uint64_t MagicImmune  = (1ULL << 11);
    constexpr uint64_t Disarmed     = (1ULL << 16);
    constexpr uint64_t Blind        = (1ULL << 23);
}

class CGameSceneNode {
public:
    Vector3 GetAbsOrigin() {
        auto addr = reinterpret_cast<uintptr_t>(this);
        if (IsBadReadPtr((void*)(addr + Offsets::SceneNode::AbsOrigin), 12))
            return {};
        return Memory::Read<Vector3>(addr + Offsets::SceneNode::AbsOrigin);
    }
};

class C_BaseEntity {
public:
    uintptr_t Address() const {
        return reinterpret_cast<uintptr_t>(this);
    }

    bool IsValid() const {
        auto addr = Address();
        return addr > 0x10000 && !IsBadReadPtr((void*)addr, 16);
    }

    // Andromeda-style identity reading:
    // entity + 0x10 = CEntityIdentity*
    // CEntityIdentity + 0x20 = m_designerName (const char*)
    const char* GetClassName() {
        if (!IsValid()) return "";

        // CEntityIdentity is at entity + 0x10
        auto identity = Memory::Read<uintptr_t>(Address() + 0x10);
        if (!identity || IsBadReadPtr((void*)identity, 0x28)) return "";

        // m_designerName at identity + 0x20
        auto namePtr = Memory::Read<uintptr_t>(identity + 0x20);
        if (!namePtr || IsBadReadPtr((void*)namePtr, 1)) return "";

        // Validate string
        auto ch = Memory::Read<uint8_t>(namePtr);
        if (ch < 0x20 || ch > 0x7E) return "";

        return reinterpret_cast<const char*>(namePtr);
    }

    // Also try m_name at identity + 0x18
    const char* GetEntityName() {
        if (!IsValid()) return "";
        auto identity = Memory::Read<uintptr_t>(Address() + 0x10);
        if (!identity || IsBadReadPtr((void*)identity, 0x20)) return "";
        auto namePtr = Memory::Read<uintptr_t>(identity + 0x18);
        if (!namePtr || IsBadReadPtr((void*)namePtr, 1)) return "";
        auto ch = Memory::Read<uint8_t>(namePtr);
        if (ch < 0x20 || ch > 0x7E) return "";
        return reinterpret_cast<const char*>(namePtr);
    }

    int GetHealth() {
        if (!IsValid()) return 0;
        return Memory::Read<int>(Address() + Offsets::Entity::Health);
    }
    int GetMaxHealth() {
        if (!IsValid()) return 0;
        return Memory::Read<int>(Address() + Offsets::Entity::MaxHealth);
    }
    uint8_t GetTeam() {
        if (!IsValid()) return 0;
        return Memory::Read<uint8_t>(Address() + Offsets::Entity::Team);
    }
    uint8_t GetLifeState() {
        if (!IsValid()) return 1;
        return Memory::Read<uint8_t>(Address() + Offsets::Entity::LifeState);
    }
    bool IsAlive() {
        return GetLifeState() == 0 && GetHealth() > 0;
    }

    CGameSceneNode* GetGameSceneNode() {
        if (!IsValid()) return nullptr;
        auto ptr = Memory::Read<uintptr_t>(Address() + Offsets::Entity::GameSceneNode);
        if (!ptr || IsBadReadPtr((void*)ptr, 0xE0)) return nullptr;
        return reinterpret_cast<CGameSceneNode*>(ptr);
    }

    Vector3 GetPosition() {
        auto node = GetGameSceneNode();
        return node ? node->GetAbsOrigin() : Vector3{};
    }

    float GetHealthPercent() {
        int mx = GetMaxHealth();
        return mx > 0 ? (float)GetHealth() / (float)mx : 0.f;
    }
};

class C_DOTABaseAbility : public C_BaseEntity {
public:
    int GetLevel() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::Ability::Level);
        return (v >= 0 && v <= 30) ? v : 0;
    }
    float GetCooldown() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::Ability::Cooldown);
        return (v >= 0.f && v < 9999.f) ? v : 0.f;
    }
    float GetCooldownLength() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::Ability::CooldownLen);
        return (v >= 0.f && v < 9999.f) ? v : 0.f;
    }
    int GetManaCost() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::Ability::ManaCost);
        return (v >= 0 && v < 9999) ? v : 0;
    }
    bool IsHidden() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Ability::IsHidden) : false; }
    bool IsActivated() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Ability::IsActivated) : false; }
    int GetMaxLevel() {
        if (!IsValid()) return 4;
        auto v = Memory::Read<int>(Address() + Offsets::Ability::MaxLevel);
        return (v > 0 && v <= 30) ? v : 4;
    }
    int GetCharges() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::Ability::Charges);
        return (v >= 0 && v < 999) ? v : 0;
    }
    float GetChargeRestoreTime() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Ability::ChargeRestore) : 0.f; }
    bool IsToggled() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Ability::ToggleState) : false; }
    bool IsAutoCast() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Ability::AutoCastState) : false; }
    bool IsInAbilityPhase() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Ability::InAbilityPhase) : false; }
    bool IsOnCooldown() { return GetCooldown() > 0.f; }
    float GetCooldownPercent() { float l = GetCooldownLength(); return l > 0.f ? GetCooldown() / l : 0.f; }
};

class C_DOTA_Item : public C_DOTABaseAbility {
public:
    int GetCurrentCharges() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::Item::CurrentCharges);
        return (v >= 0 && v < 999) ? v : 0;
    }
    int GetSecondaryCharges() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Item::SecondaryCharges) : 0; }
    int GetMaxCharges() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Item::MaxCharges) : 0; }
    bool IsPurchasable() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Item::Purchasable) : false; }
    bool IsDroppable() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Item::Droppable) : false; }
    float GetPurchaseTime() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Item::PurchaseTime) : 0.f; }
    int GetPlayerOwnerID() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Item::PlayerOwnerID) : -1; }
};

class C_DOTA_BaseNPC : public C_BaseEntity {
public:
    float GetMana() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::BaseNPC::Mana);
        return (v >= 0.f && v < 99999.f) ? v : 0.f;
    }
    float GetMaxMana() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::BaseNPC::MaxMana);
        return (v >= 0.f && v < 99999.f) ? v : 0.f;
    }
    float GetManaPercent() { float mx = GetMaxMana(); return mx > 0.f ? GetMana() / mx : 0.f; }
    float GetManaRegen() { return IsValid() ? Memory::Read<float>(Address() + Offsets::BaseNPC::ManaRegen) : 0.f; }
    float GetHealthRegen() { return IsValid() ? Memory::Read<float>(Address() + Offsets::BaseNPC::HealthRegen) : 0.f; }
    int GetLevel() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::BaseNPC::Level);
        return (v > 0 && v <= 30) ? v : 0;
    }
    int GetAttackRange() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::BaseNPC::AttackRange);
        return (v >= 0 && v < 9999) ? v : 0;
    }
    int GetMoveSpeed() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::BaseNPC::MoveSpeed);
        return (v >= 0 && v < 9999) ? v : 0;
    }
    int GetDayVision() { return IsValid() ? Memory::Read<int>(Address() + Offsets::BaseNPC::DayVision) : 0; }
    int GetNightVision() { return IsValid() ? Memory::Read<int>(Address() + Offsets::BaseNPC::NightVision) : 0; }
    bool IsIllusion() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsIllusion) : false; }
    bool IsPhantom() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsPhantom) : false; }
    bool IsAncient() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsAncient) : false; }
    bool IsSummoned() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsSummoned) : false; }
    uint64_t GetUnitState() { return IsValid() ? Memory::Read<uint64_t>(Address() + Offsets::BaseNPC::UnitState64) : 0; }
    bool HasState(uint64_t f) { return (GetUnitState() & f) != 0; }
    bool IsStunned() { return HasState(UnitState::Stunned); }
    bool IsRooted() { return HasState(UnitState::Rooted); }
    bool IsSilenced() { return HasState(UnitState::Silenced); }
    bool IsHexed() { return HasState(UnitState::Hexed); }
    bool IsInvisible() { return HasState(UnitState::Invisible); }
    bool IsInvulnerable() { return HasState(UnitState::Invulnerable); }
    bool IsMagicImmune() { return HasState(UnitState::MagicImmune); }
    bool IsMuted() { return HasState(UnitState::Muted); }
    bool IsDisarmed() { return HasState(UnitState::Disarmed); }
    bool IsBlind() { return HasState(UnitState::Blind); }

    const char* GetUnitName() {
        if (!IsValid()) return "unknown";
        auto ptr = Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::UnitName);
        if (!ptr || IsBadReadPtr((void*)ptr, 4)) return "unknown";
        auto ch = Memory::Read<uint8_t>(ptr);
        if (ch < 0x20 || ch > 0x7E) return "unknown";
        return reinterpret_cast<const char*>(ptr);
    }

    uintptr_t GetModifierManager() {
        return IsValid() ? Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::ModifierManager) : 0;
    }

    uint32_t GetAbilityHandle(int index) {
        if (!IsValid()) return 0xFFFFFFFF;
        auto data = Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::AbilityList);
        if (!data || IsBadReadPtr((void*)(data + index * 4), 4)) return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + index * 0x4);
    }

    int GetAbilityCount() {
        if (!IsValid()) return 0;
        auto v = Memory::Read<int>(Address() + Offsets::BaseNPC::AbilityList + 0x10);
        return (v >= 0 && v <= 35) ? v : 0;
    }

    uint32_t GetItemHandle(int slot) {
        if (!IsValid()) return 0xFFFFFFFF;
        auto data = Memory::Read<uintptr_t>(
            Address() + Offsets::BaseNPC::Inventory + Offsets::Inventory::Items);
        if (!data || IsBadReadPtr((void*)(data + slot * 4), 4)) return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + slot * 0x4);
    }
};

class C_DOTA_BaseNPC_Hero : public C_DOTA_BaseNPC {
public:
    int GetHeroID() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::HeroID) : 0; }
    int GetPlayerID() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::PlayerID) : -1; }
    int GetCurrentXP() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::CurrentXP) : 0; }
    int GetAbilityPoints() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::AbilityPoints) : 0; }
    float GetRespawnTime() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::RespawnTime) : 0.f; }
    float GetStrength() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Strength) : 0.f; }
    float GetAgility() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Agility) : 0.f; }
    float GetIntellect() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Intellect) : 0.f; }
    float GetStrengthTotal() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::Hero::StrengthTotal);
        return (v >= 0.f && v < 9999.f) ? v : 0.f;
    }
    float GetAgilityTotal() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::Hero::AgilityTotal);
        return (v >= 0.f && v < 9999.f) ? v : 0.f;
    }
    float GetIntellectTotal() {
        if (!IsValid()) return 0.f;
        auto v = Memory::Read<float>(Address() + Offsets::Hero::IntellectTotal);
        return (v >= 0.f && v < 9999.f) ? v : 0.f;
    }
    int GetPrimaryAttribute() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::PrimaryAttribute) : -1; }
    bool IsReincarnating() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::Hero::Reincarnating) : false; }
    bool IsReplicating() {
        if (!IsValid()) return false;
        auto h = Memory::Read<uint32_t>(Address() + Offsets::Hero::ReplicatingHero);
        return h != 0xFFFFFFFF && h != 0;
    }
    const char* GetPrimaryAttributeName() {
        switch (GetPrimaryAttribute()) {
            case 0: return "STR"; case 1: return "AGI";
            case 2: return "INT"; case 3: return "UNI";
            default: return "???";
        }
    }
};

class C_DOTAPlayerController : public C_BaseEntity {
public:
    int GetPlayerID() { return IsValid() ? Memory::Read<int>(Address() + Offsets::PlayerController::PlayerID) : -1; }
    uint32_t GetAssignedHeroHandle() { return IsValid() ? Memory::Read<uint32_t>(Address() + Offsets::PlayerController::AssignedHero) : 0xFFFFFFFF; }
    float GetCameraZoom() { return IsValid() ? Memory::Read<float>(Address() + Offsets::PlayerController::CameraZoom) : 0.f; }
    void SetCameraZoom(float z) { if (IsValid()) Memory::Write<float>(Address() + Offsets::PlayerController::CameraZoom, z); }
};

struct AbilityInfo {
    std::string name;
    int level = 0;
    float cooldown = 0.f, cooldownMax = 0.f;
    int manaCost = 0, charges = 0;
    float chargeRestore = 0.f;
    bool isHidden = false, isActivated = true, isUltimate = false, isToggled = false, inPhase = false;
    bool isOnCooldown() const { return cooldown > 0.f; }
    float getCooldownPercent() const { return cooldownMax > 0.f ? cooldown / cooldownMax : 0.f; }
};

struct ItemInfo {
    std::string name;
    int slot = -1, charges = 0;
    float cooldown = 0.f, cooldownMax = 0.f;
    bool isOnCooldown() const { return cooldown > 0.f; }
};

struct HeroData {
    C_DOTA_BaseNPC_Hero* entity = nullptr;
    Vector3 pos;
    uint8_t team = 0;
    int health = 0, maxHP = 0;
    float mana = 0.f, maxMana = 0.f, hpRegen = 0.f, mpRegen = 0.f;
    int level = 0, heroID = 0, playerID = 0;
    bool alive = false, illusion = false, visible = true;
    int moveSpeed = 0, attackRange = 0;
    std::string name, primaryAttr;
    float str = 0.f, agi = 0.f, intel = 0.f;
    bool stunned = false, silenced = false, hexed = false, rooted = false;
    bool invisible = false, invulnerable = false, magicImmune = false, disarmed = false, muted = false;
    std::vector<AbilityInfo> abilities;
    std::vector<ItemInfo> items;
};
