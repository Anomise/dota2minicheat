#include "camera_hack.h"
#include "../sdk/interfaces.h"
#include "../config.h"
#include "../utils/memory.h"
#include <cmath>

namespace CameraHack {
    static float origDist = 1200.f, origFOV = 0.f;
    static bool init = false, wasOn = false;

    void Initialize() {
        if (SDK::pCameraInstance) {
            origDist = Memory::Read<float>(SDK::pCameraInstance + Offsets::Camera::Distance);
            origFOV  = Memory::Read<float>(SDK::pCameraInstance + Offsets::Camera::FOV);
            init = true;
        }
    }

    void OnFrame() {
        auto& c = Config::Get();
        if (!c.camerHack) { if (wasOn) { Restore(); wasOn = false; } return; }
        wasOn = true;

        if (SDK::pCameraInstance) {
            float cur = Memory::Read<float>(SDK::pCameraInstance + Offsets::Camera::Distance);
            if (std::abs(cur - c.cameraDistance) > 0.5f)
                Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::Distance, c.cameraDistance);
            if (c.cameraFOV != 0.f)
                Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FOV, c.cameraFOV);
            if (c.cameraFog) {
                Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FogEnd, 99999.f);
                Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FogStart, 99999.f);
            }
        }

        auto ctrl = SDK::GetLocalPlayerController();
        if (ctrl) {
            float desired = c.cameraDistance - 1134.f;
            if (std::abs(ctrl->GetCameraZoom() - desired) > 1.f)
                ctrl->SetCameraZoom(desired);
        }
    }

    void Restore() {
        if (SDK::pCameraInstance && init) {
            Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::Distance, origDist);
            Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FOV, origFOV);
            Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FogEnd, 5000.f);
            Memory::Write<float>(SDK::pCameraInstance + Offsets::Camera::FogStart, 3000.f);
        }
        auto ctrl = SDK::GetLocalPlayerController();
        if (ctrl) ctrl->SetCameraZoom(0.f);
    }
}
