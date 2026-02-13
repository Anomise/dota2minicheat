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
        if (IsBadReadPtr(reinterpret_cast<void*>(addr + Offsets::SceneNode::AbsOrigin), sizeof(Vector3)))
            return {};
        return Memory::Read<Vector3>(addr + Offsets::SceneNode::AbsOrigin);
    }
};

class C_BaseEntity {
public:
    uintptr_t Address() const { return reinterpret_cast<uintptr_t>(this); }

    bool IsValid() const {
        return Address() > 0x10000 && !IsBadReadPtr(reinterpret_cast<void*>(Address()), 16);
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
        if (!ptr || IsBadReadPtr(reinterpret_cast<void*>(ptr), 0x100)) return nullptr;
        return reinterpret_cast<CGameSceneNode*>(ptr);
    }

    Vector3 GetPosition() {
        auto node = GetGameSceneNode();
        if (!node) return {};
        return node->GetAbsOrigin();
    }

    float GetHealthPercent() {
        int mx = GetMaxHealth();
        return mx > 0 ? (float)GetHealth() / (float)mx : 0.f;
    }

    const char* GetClassName() {
        if (!IsValid()) return "";
        // CEntityInstance at +0x10 is CEntityIdentity*
        auto identity = Memory::Read<uintptr_t>(Address() + 0x10);
        if (!identity || IsBadReadPtr(reinterpret_cast<void*>(identity), 0x20)) return "";

        // CEntityIdentity -> m_designerName at +0x20
        auto namePtr = Memory::Read<uintptr_t>(identity + 0x20);
        if (!namePtr || IsBadReadPtr(reinterpret_cast<void*>(namePtr), 4)) return "";

        // Verify the string looks valid
        auto firstByte = Memory::Read<uint8_t>(namePtr);
        if (firstByte < 0x20 || firstByte > 0x7E) return "";

        return reinterpret_cast<const char*>(namePtr);
    }
};

class C_DOTABaseAbility : public C_BaseEntity {
public:
    int GetLevel() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::Ability::Level);
        return (val >= 0 && val <= 30) ? val : 0;
    }
    float GetCooldown() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::Ability::Cooldown);
        return (val >= 0.f && val < 9999.f) ? val : 0.f;
    }
    float GetCooldownLength() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::Ability::CooldownLen);
        return (val >= 0.f && val < 9999.f) ? val : 0.f;
    }
    int GetManaCost() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::Ability::ManaCost);
        return (val >= 0 && val < 9999) ? val : 0;
    }
    bool IsHidden() {
        if (!IsValid()) return false;
        return Memory::Read<bool>(Address() + Offsets::Ability::IsHidden);
    }
    bool IsActivated() {
        if (!IsValid()) return false;
        return Memory::Read<bool>(Address() + Offsets::Ability::IsActivated);
    }
    int GetMaxLevel() {
        if (!IsValid()) return 4;
        auto val = Memory::Read<int>(Address() + Offsets::Ability::MaxLevel);
        return (val > 0 && val <= 30) ? val : 4;
    }
    int GetCharges() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::Ability::Charges);
        return (val >= 0 && val < 999) ? val : 0;
    }
    float GetChargeRestoreTime() {
        if (!IsValid()) return 0.f;
        return Memory::Read<float>(Address() + Offsets::Ability::ChargeRestore);
    }
    bool IsToggled() {
        if (!IsValid()) return false;
        return Memory::Read<bool>(Address() + Offsets::Ability::ToggleState);
    }
    bool IsAutoCast() {
        if (!IsValid()) return false;
        return Memory::Read<bool>(Address() + Offsets::Ability::AutoCastState);
    }
    bool IsInAbilityPhase() {
        if (!IsValid()) return false;
        return Memory::Read<bool>(Address() + Offsets::Ability::InAbilityPhase);
    }
    bool IsOnCooldown() { return GetCooldown() > 0.f; }
    float GetCooldownPercent() {
        float l = GetCooldownLength();
        return l > 0.f ? GetCooldown() / l : 0.f;
    }
};

class C_DOTA_Item : public C_DOTABaseAbility {
public:
    int GetCurrentCharges() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::Item::CurrentCharges);
        return (val >= 0 && val < 999) ? val : 0;
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
        auto val = Memory::Read<float>(Address() + Offsets::BaseNPC::Mana);
        return (val >= 0.f && val < 99999.f) ? val : 0.f;
    }
    float GetMaxMana() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::BaseNPC::MaxMana);
        return (val >= 0.f && val < 99999.f) ? val : 0.f;
    }
    float GetManaPercent() { float mx = GetMaxMana(); return mx > 0.f ? GetMana() / mx : 0.f; }
    float GetManaRegen() {
        if (!IsValid()) return 0.f;
        return Memory::Read<float>(Address() + Offsets::BaseNPC::ManaRegen);
    }
    float GetHealthRegen() {
        if (!IsValid()) return 0.f;
        return Memory::Read<float>(Address() + Offsets::BaseNPC::HealthRegen);
    }
    int GetLevel() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::BaseNPC::Level);
        return (val > 0 && val <= 30) ? val : 0;
    }
    int GetAttackRange() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::BaseNPC::AttackRange);
        return (val >= 0 && val < 9999) ? val : 0;
    }
    int GetMoveSpeed() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::BaseNPC::MoveSpeed);
        return (val >= 0 && val < 9999) ? val : 0;
    }
    int GetDayVision() { return IsValid() ? Memory::Read<int>(Address() + Offsets::BaseNPC::DayVision) : 0; }
    int GetNightVision() { return IsValid() ? Memory::Read<int>(Address() + Offsets::BaseNPC::NightVision) : 0; }
    bool IsIllusion() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsIllusion) : false; }
    bool IsPhantom() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsPhantom) : false; }
    bool IsAncient() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsAncient) : false; }
    bool IsSummoned() { return IsValid() ? Memory::Read<bool>(Address() + Offsets::BaseNPC::IsSummoned) : false; }

    uint64_t GetUnitState() {
        if (!IsValid()) return 0;
        return Memory::Read<uint64_t>(Address() + Offsets::BaseNPC::UnitState64);
    }
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
        if (!ptr || IsBadReadPtr(reinterpret_cast<void*>(ptr), 4)) return "unknown";
        auto firstByte = Memory::Read<uint8_t>(ptr);
        if (firstByte < 0x20 || firstByte > 0x7E) return "unknown";
        return reinterpret_cast<const char*>(ptr);
    }

    uintptr_t GetModifierManager() {
        if (!IsValid()) return 0;
        return Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::ModifierManager);
    }

    uint32_t GetAbilityHandle(int index) {
        if (!IsValid()) return 0xFFFFFFFF;
        auto data = Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::AbilityList);
        if (!data || IsBadReadPtr(reinterpret_cast<void*>(data), (index + 1) * 4))
            return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + index * 0x4);
    }

    int GetAbilityCount() {
        if (!IsValid()) return 0;
        auto val = Memory::Read<int>(Address() + Offsets::BaseNPC::AbilityList + 0x10);
        return (val >= 0 && val <= 35) ? val : 0;
    }

    uint32_t GetItemHandle(int slot) {
        if (!IsValid()) return 0xFFFFFFFF;
        auto data = Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::Inventory + Offsets::Inventory::Items);
        if (!data || IsBadReadPtr(reinterpret_cast<void*>(data), (slot + 1) * 4))
            return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + slot * 0x4);
    }
};

class C_DOTA_BaseNPC_Hero : public C_DOTA_BaseNPC {
public:
    int GetHeroID() {
        if (!IsValid()) return 0;
        return Memory::Read<int>(Address() + Offsets::Hero::HeroID);
    }
    int GetPlayerID() {
        if (!IsValid()) return -1;
        return Memory::Read<int>(Address() + Offsets::Hero::PlayerID);
    }
    int GetCurrentXP() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::CurrentXP) : 0; }
    int GetAbilityPoints() { return IsValid() ? Memory::Read<int>(Address() + Offsets::Hero::AbilityPoints) : 0; }
    float GetRespawnTime() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::RespawnTime) : 0.f; }

    float GetStrength() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Strength) : 0.f; }
    float GetAgility() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Agility) : 0.f; }
    float GetIntellect() { return IsValid() ? Memory::Read<float>(Address() + Offsets::Hero::Intellect) : 0.f; }
    float GetStrengthTotal() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::Hero::StrengthTotal);
        return (val >= 0.f && val < 9999.f) ? val : 0.f;
    }
    float GetAgilityTotal() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::Hero::AgilityTotal);
        return (val >= 0.f && val < 9999.f) ? val : 0.f;
    }
    float GetIntellectTotal() {
        if (!IsValid()) return 0.f;
        auto val = Memory::Read<float>(Address() + Offsets::Hero::IntellectTotal);
        return (val >= 0.f && val < 9999.f) ? val : 0.f;
    }

    int GetPrimaryAttribute() {
        if (!IsValid()) return -1;
        return Memory::Read<int>(Address() + Offsets::Hero::PrimaryAttribute);
    }

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
    int GetPlayerID() {
        if (!IsValid()) return -1;
        return Memory::Read<int>(Address() + Offsets::PlayerController::PlayerID);
    }
    uint32_t GetAssignedHeroHandle() {
        if (!IsValid()) return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(Address() + Offsets::PlayerController::AssignedHero);
    }
    float GetCameraZoom() {
        if (!IsValid()) return 0.f;
        return Memory::Read<float>(Address() + Offsets::PlayerController::CameraZoom);
    }
    void SetCameraZoom(float z) {
        if (!IsValid()) return;
        Memory::Write<float>(Address() + Offsets::PlayerController::CameraZoom, z);
    }
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
