#pragma once
#include <cstdint>

namespace Offsets {
    constexpr const char* CLIENT_DLL   = "client.dll";
    constexpr const char* ENGINE_DLL   = "engine2.dll";
    constexpr const char* SCHEMASYS    = "schemasystem.dll";
    constexpr const char* TIER0        = "tier0.dll";
    constexpr const char* INPUTSYS     = "inputsystem.dll";
    constexpr const char* RENDERSYS    = "rendersystemdx11.dll";

    namespace Interfaces {
        constexpr const char* GameEntitySystem      = "GameEntitySystem";
        constexpr const char* Source2Client          = "Source2Client";
        constexpr const char* Source2EngineToClient  = "Source2EngineToClient";
        constexpr const char* SchemaSystem           = "SchemaSystem_001";
        constexpr const char* InputSystemVersion     = "InputSystemVersion001";
    }

    namespace EntitySystem {
        constexpr uintptr_t EntityList         = 0x10;
        constexpr uintptr_t HighestEntityIndex = 0x1520;
    }

    namespace Entity {
        constexpr uintptr_t Health         = 0x354;
        constexpr uintptr_t MaxHealth      = 0x350;
        constexpr uintptr_t Team           = 0x3F3;
        constexpr uintptr_t LifeState      = 0x35C;
        constexpr uintptr_t GameSceneNode  = 0x338;
        constexpr uintptr_t SchemaName     = 0x18;
    }

    namespace SceneNode {
        constexpr uintptr_t AbsOrigin      = 0xD0;
        constexpr uintptr_t AbsRotation    = 0xDC;
    }

    namespace BaseNPC {
        constexpr uintptr_t Mana               = 0xCD4;
        constexpr uintptr_t MaxMana            = 0xCD8;
        constexpr uintptr_t ManaRegen          = 0x16F0;
        constexpr uintptr_t HealthRegen        = 0x16F4;
        constexpr uintptr_t Level              = 0xC7C;
        constexpr uintptr_t UnitName           = 0xD48;
        constexpr uintptr_t Inventory          = 0x1168;
        constexpr uintptr_t AbilityList        = 0xD00;
        constexpr uintptr_t IsIllusion         = 0xCFC;
        constexpr uintptr_t AttackRange        = 0xCA8;
        constexpr uintptr_t MoveSpeed          = 0xCBC;
        constexpr uintptr_t DayVision          = 0xDE8;
        constexpr uintptr_t NightVision        = 0xDEC;
        constexpr uintptr_t IsPhantom          = 0xC60;
        constexpr uintptr_t IsAncient          = 0xC80;
        constexpr uintptr_t IsSummoned         = 0xC8B;
        constexpr uintptr_t ModifierManager    = 0xE08;
        constexpr uintptr_t UnitState64        = 0x1260;
    }

    namespace Hero {
        constexpr uintptr_t HeroID             = 0x1DA0;
        constexpr uintptr_t PlayerID           = 0x1AF8;
        constexpr uintptr_t CurrentXP          = 0x1A5C;
        constexpr uintptr_t AbilityPoints      = 0x1A60;
        constexpr uintptr_t RespawnTime        = 0x1A68;
        constexpr uintptr_t Strength           = 0x1A70;
        constexpr uintptr_t Agility            = 0x1A74;
        constexpr uintptr_t Intellect          = 0x1A78;
        constexpr uintptr_t StrengthTotal      = 0x1A7C;
        constexpr uintptr_t AgilityTotal       = 0x1A80;
        constexpr uintptr_t IntellectTotal     = 0x1A84;
        constexpr uintptr_t ReplicatingHero    = 0x1B0C;
        constexpr uintptr_t Reincarnating      = 0x1B10;
        constexpr uintptr_t PrimaryAttribute   = 0x1B1C;
    }

    namespace Ability {
        constexpr uintptr_t Level          = 0x630;
        constexpr uintptr_t Cooldown       = 0x640;
        constexpr uintptr_t CooldownLen    = 0x644;
        constexpr uintptr_t ManaCost       = 0x648;
        constexpr uintptr_t IsHidden       = 0x61F;
        constexpr uintptr_t IsActivated    = 0x621;
        constexpr uintptr_t MaxLevel       = 0x608;
        constexpr uintptr_t Charges        = 0x668;
        constexpr uintptr_t ChargeRestore  = 0x66C;
        constexpr uintptr_t ToggleState    = 0x635;
        constexpr uintptr_t AutoCastState  = 0x64C;
        constexpr uintptr_t InAbilityPhase = 0x63C;
    }

    namespace Item {
        constexpr uintptr_t CurrentCharges     = 0x6E0;
        constexpr uintptr_t SecondaryCharges   = 0x6E4;
        constexpr uintptr_t MaxCharges         = 0x6E8;
        constexpr uintptr_t Cooldown           = 0x640;
        constexpr uintptr_t CooldownLen        = 0x644;
        constexpr uintptr_t Purchasable        = 0x6BD;
        constexpr uintptr_t Droppable          = 0x6BC;
        constexpr uintptr_t PurchaseTime       = 0x6F0;
        constexpr uintptr_t PlayerOwnerID      = 0x720;
    }

    namespace Inventory {
        constexpr uintptr_t Items              = 0x20;
        constexpr uintptr_t StashEnabled       = 0xAD;
        constexpr uintptr_t InventoryParent    = 0xA8;
    }

    namespace PlayerController {
        constexpr uintptr_t PlayerID           = 0x908;
        constexpr uintptr_t AssignedHero       = 0x90C;
        constexpr uintptr_t SelectedUnits      = 0x9C0;
        constexpr uintptr_t QueryUnit          = 0x9F4;
        constexpr uintptr_t CameraZoom         = 0xC5C;
    }

    namespace Camera {
        constexpr uintptr_t Distance     = 0x4;
        constexpr uintptr_t FOV          = 0x8;
        constexpr uintptr_t FogStart     = 0x30;
        constexpr uintptr_t FogEnd       = 0x34;
    }

    namespace GameRules {
        constexpr uintptr_t GameTime     = 0x30;
        constexpr uintptr_t GameMode     = 0x68;
        constexpr uintptr_t PreGameTime  = 0x34;
        constexpr uintptr_t IsPaused     = 0x48;
    }

    namespace Patterns {
        constexpr const char* GameEntitySystem  = "48 8B 1D ?? ?? ?? ?? 48 85 DB 74 ?? 48 8B CB";
        constexpr const char* CameraInstance    = "48 8D 05 ?? ?? ?? ?? 48 89 01 F3 0F 10 05";
        constexpr const char* GameRules         = "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 48";
        constexpr const char* ViewMatrix        = "48 8D 05 ?? ?? ?? ?? 48 C7 05";
        constexpr const char* LocalPlayer       = "48 89 05 ?? ?? ?? ?? 48 8B 4E";
    }
}
