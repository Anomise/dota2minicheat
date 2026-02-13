#pragma once
#include "../utils/math.h"
#include "../utils/memory.h"
#include "offsets.h"
#include <string>
#include <vector>
#include <cstring>

namespace UnitState {
    constexpr uint64_t Stunned       = (1ULL << 2);
    constexpr uint64_t Rooted        = (1ULL << 5);
    constexpr uint64_t Silenced      = (1ULL << 3);
    constexpr uint64_t Hexed         = (1ULL << 6);
    constexpr uint64_t Invisible     = (1ULL << 8);
    constexpr uint64_t Invulnerable  = (1ULL << 10);
    constexpr uint64_t MagicImmune   = (1ULL << 11);
    constexpr uint64_t Muted         = (1ULL << 4);
    constexpr uint64_t Disarmed      = (1ULL << 16);
    constexpr uint64_t Blind         = (1ULL << 23);
}

class CGameSceneNode {
public:
    Vector3 GetAbsOrigin() {
        return Memory::Read<Vector3>(reinterpret_cast<uintptr_t>(this) + Offsets::SceneNode::AbsOrigin);
    }
    Vector3 GetAbsRotation() {
        return Memory::Read<Vector3>(reinterpret_cast<uintptr_t>(this) + Offsets::SceneNode::AbsRotation);
    }
};

class C_BaseEntity {
public:
    uintptr_t Address() const { return reinterpret_cast<uintptr_t>(this); }
    int GetHealth() { return Memory::Read<int>(Address() + Offsets::Entity::Health); }
    int GetMaxHealth() { return Memory::Read<int>(Address() + Offsets::Entity::MaxHealth); }
    uint8_t GetTeam() { return Memory::Read<uint8_t>(Address() + Offsets::Entity::Team); }
    uint8_t GetLifeState() { return Memory::Read<uint8_t>(Address() + Offsets::Entity::LifeState); }
    bool IsAlive() { return GetLifeState() == 0 && GetHealth() > 0; }

    CGameSceneNode* GetGameSceneNode() {
        return Memory::Read<CGameSceneNode*>(Address() + Offsets::Entity::GameSceneNode);
    }

    Vector3 GetPosition() {
        auto node = GetGameSceneNode();
        if (!node) return {};
        return node->GetAbsOrigin();
    }

    float GetHealthPercent() {
        int max = GetMaxHealth();
        if (max <= 0) return 0.f;
        return static_cast<float>(GetHealth()) / static_cast<float>(max);
    }

    const char* GetClassName() {
        auto identity = Memory::Read<uintptr_t>(Address() + 0x10);
        if (!identity) return "";
        auto designerName = Memory::Read<uintptr_t>(identity + Offsets::Entity::SchemaName);
        if (!designerName) return "";
        if (IsBadReadPtr(reinterpret_cast<void*>(designerName), 4)) return "";
        return reinterpret_cast<const char*>(designerName);
    }
};

class C_DOTABaseAbility : public C_BaseEntity {
public:
    int GetLevel() { return Memory::Read<int>(Address() + Offsets::Ability::Level); }
    float GetCooldown() { return Memory::Read<float>(Address() + Offsets::Ability::Cooldown); }
    float GetCooldownLength() { return Memory::Read<float>(Address() + Offsets::Ability::CooldownLen); }
    int GetManaCost() { return Memory::Read<int>(Address() + Offsets::Ability::ManaCost); }
    bool IsHidden() { return Memory::Read<bool>(Address() + Offsets::Ability::IsHidden); }
    bool IsActivated() { return Memory::Read<bool>(Address() + Offsets::Ability::IsActivated); }
    int GetMaxLevel() { return Memory::Read<int>(Address() + Offsets::Ability::MaxLevel); }
    int GetCharges() { return Memory::Read<int>(Address() + Offsets::Ability::Charges); }
    float GetChargeRestoreTime() { return Memory::Read<float>(Address() + Offsets::Ability::ChargeRestore); }
    bool IsToggled() { return Memory::Read<bool>(Address() + Offsets::Ability::ToggleState); }
    bool IsAutoCast() { return Memory::Read<bool>(Address() + Offsets::Ability::AutoCastState); }
    bool IsInAbilityPhase() { return Memory::Read<bool>(Address() + Offsets::Ability::InAbilityPhase); }
    bool IsOnCooldown() { return GetCooldown() > 0.f; }
    float GetCooldownPercent() {
        float len = GetCooldownLength();
        if (len <= 0.f) return 0.f;
        return GetCooldown() / len;
    }
};

class C_DOTA_Item : public C_DOTABaseAbility {
public:
    int GetCurrentCharges() { return Memory::Read<int>(Address() + Offsets::Item::CurrentCharges); }
    int GetSecondaryCharges() { return Memory::Read<int>(Address() + Offsets::Item::SecondaryCharges); }
    int GetMaxCharges() { return Memory::Read<int>(Address() + Offsets::Item::MaxCharges); }
    bool IsPurchasable() { return Memory::Read<bool>(Address() + Offsets::Item::Purchasable); }
    bool IsDroppable() { return Memory::Read<bool>(Address() + Offsets::Item::Droppable); }
    float GetPurchaseTime() { return Memory::Read<float>(Address() + Offsets::Item::PurchaseTime); }
    int GetPlayerOwnerID() { return Memory::Read<int>(Address() + Offsets::Item::PlayerOwnerID); }
};

class C_DOTA_BaseNPC : public C_BaseEntity {
public:
    float GetMana() { return Memory::Read<float>(Address() + Offsets::BaseNPC::Mana); }
    float GetMaxMana() { return Memory::Read<float>(Address() + Offsets::BaseNPC::MaxMana); }
    float GetManaPercent() { float max = GetMaxMana(); return max > 0.f ? GetMana() / max : 0.f; }
    float GetManaRegen() { return Memory::Read<float>(Address() + Offsets::BaseNPC::ManaRegen); }
    float GetHealthRegen() { return Memory::Read<float>(Address() + Offsets::BaseNPC::HealthRegen); }
    int GetLevel() { return Memory::Read<int>(Address() + Offsets::BaseNPC::Level); }
    int GetAttackRange() { return Memory::Read<int>(Address() + Offsets::BaseNPC::AttackRange); }
    int GetMoveSpeed() { return Memory::Read<int>(Address() + Offsets::BaseNPC::MoveSpeed); }
    int GetDayVision() { return Memory::Read<int>(Address() + Offsets::BaseNPC::DayVision); }
    int GetNightVision() { return Memory::Read<int>(Address() + Offsets::BaseNPC::NightVision); }
    bool IsIllusion() { return Memory::Read<bool>(Address() + Offsets::BaseNPC::IsIllusion); }
    bool IsPhantom() { return Memory::Read<bool>(Address() + Offsets::BaseNPC::IsPhantom); }
    bool IsAncient() { return Memory::Read<bool>(Address() + Offsets::BaseNPC::IsAncient); }
    bool IsSummoned() { return Memory::Read<bool>(Address() + Offsets::BaseNPC::IsSummoned); }
    uint64_t GetUnitState() { return Memory::Read<uint64_t>(Address() + Offsets::BaseNPC::UnitState64); }
    bool HasState(uint64_t flag) { return (GetUnitState() & flag) != 0; }
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
        auto ptr = Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::UnitName);
        if (!ptr || IsBadReadPtr(reinterpret_cast<void*>(ptr), 4)) return "unknown";
        return reinterpret_cast<const char*>(ptr);
    }

    uintptr_t GetModifierManager() {
        return Memory::Read<uintptr_t>(Address() + Offsets::BaseNPC::ModifierManager);
    }

    uint32_t GetAbilityHandle(int index) {
        auto abilityList = Address() + Offsets::BaseNPC::AbilityList;
        auto data = Memory::Read<uintptr_t>(abilityList);
        if (!data) return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + index * 0x4);
    }

    int GetAbilityCount() {
        auto abilityList = Address() + Offsets::BaseNPC::AbilityList;
        return Memory::Read<int>(abilityList + 0x10);
    }

    uint32_t GetItemHandle(int slot) {
        auto inventoryAddr = Address() + Offsets::BaseNPC::Inventory;
        auto itemsAddr = inventoryAddr + Offsets::Inventory::Items;
        auto data = Memory::Read<uintptr_t>(itemsAddr);
        if (!data) return 0xFFFFFFFF;
        return Memory::Read<uint32_t>(data + slot * 0x4);
    }
};

class C_DOTA_BaseNPC_Hero : public C_DOTA_BaseNPC {
public:
    int GetHeroID() { return Memory::Read<int>(Address() + Offsets::Hero::HeroID); }
    int GetPlayerID() { return Memory::Read<int>(Address() + Offsets::Hero::PlayerID); }
    int GetCurrentXP() { return Memory::Read<int>(Address() + Offsets::Hero::CurrentXP); }
    int GetAbilityPoints() { return Memory::Read<int>(Address() + Offsets::Hero::AbilityPoints); }
    float GetRespawnTime() { return Memory::Read<float>(Address() + Offsets::Hero::RespawnTime); }
    float GetStrength() { return Memory::Read<float>(Address() + Offsets::Hero::Strength); }
    float GetAgility() { return Memory::Read<float>(Address() + Offsets::Hero::Agility); }
    float GetIntellect() { return Memory::Read<float>(Address() + Offsets::Hero::Intellect); }
    float GetStrengthTotal() { return Memory::Read<float>(Address() + Offsets::Hero::StrengthTotal); }
    float GetAgilityTotal() { return Memory::Read<float>(Address() + Offsets::Hero::AgilityTotal); }
    float GetIntellectTotal() { return Memory::Read<float>(Address() + Offsets::Hero::IntellectTotal); }
    int GetPrimaryAttribute() { return Memory::Read<int>(Address() + Offsets::Hero::PrimaryAttribute); }
    bool IsReincarnating() { return Memory::Read<bool>(Address() + Offsets::Hero::Reincarnating); }

    bool IsReplicating() {
        auto handle = Memory::Read<uint32_t>(Address() + Offsets::Hero::ReplicatingHero);
        return handle != 0xFFFFFFFF && handle != 0;
    }

    const char* GetPrimaryAttributeName() {
        switch (GetPrimaryAttribute()) {
            case 0: return "STR";
            case 1: return "AGI";
            case 2: return "INT";
            case 3: return "UNI";
            default: return "???";
        }
    }
};

class C_DOTAPlayerController : public C_BaseEntity {
public:
    int GetPlayerID() { return Memory::Read<int>(Address() + Offsets::PlayerController::PlayerID); }
    uint32_t GetAssignedHeroHandle() { return Memory::Read<uint32_t>(Address() + Offsets::PlayerController::AssignedHero); }
    float GetCameraZoom() { return Memory::Read<float>(Address() + Offsets::PlayerController::CameraZoom); }
    void SetCameraZoom(float zoom) { Memory::Write<float>(Address() + Offsets::PlayerController::CameraZoom, zoom); }
};

struct AbilityInfo {
    std::string name;
    int level = 0;
    float cooldown = 0.f;
    float cooldownMax = 0.f;
    int manaCost = 0;
    int charges = 0;
    float chargeRestore = 0.f;
    bool isHidden = false;
    bool isActivated = true;
    bool isUltimate = false;
    bool isToggled = false;
    bool inPhase = false;
    bool isOnCooldown() const { return cooldown > 0.f; }
    float getCooldownPercent() const {
        if (cooldownMax <= 0.f) return 0.f;
        return cooldown / cooldownMax;
    }
};

struct ItemInfo {
    std::string name;
    int slot = -1;
    int charges = 0;
    float cooldown = 0.f;
    float cooldownMax = 0.f;
    bool isOnCooldown() const { return cooldown > 0.f; }
};

struct HeroData {
    C_DOTA_BaseNPC_Hero* entity = nullptr;
    Vector3 pos;
    uint8_t team = 0;
    int health = 0;
    int maxHP = 0;
    float mana = 0.f;
    float maxMana = 0.f;
    float hpRegen = 0.f;
    float mpRegen = 0.f;
    int level = 0;
    int heroID = 0;
    int playerID = 0;
    bool alive = false;
    bool illusion = false;
    bool visible = true;
    int moveSpeed = 0;
    int attackRange = 0;
    std::string name;
    std::string primaryAttr;
    float str = 0.f, agi = 0.f, intel = 0.f;
    bool stunned = false;
    bool silenced = false;
    bool hexed = false;
    bool rooted = false;
    bool invisible = false;
    bool invulnerable = false;
    bool magicImmune = false;
    bool disarmed = false;
    bool muted = false;
    std::vector<AbilityInfo> abilities;
    std::vector<ItemInfo> items;
};
