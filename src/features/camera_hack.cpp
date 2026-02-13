#include "camera_hack.h"
#include "../sdk/interfaces.h"
#include "../config.h"
#include "../utils/memory.h"
#include <cmath>

namespace CameraHack {
    static bool wasEnabled = false;
    static float originalZoom = 0.f;
    static bool savedOriginal = false;

    void Initialize() {
        // Nothing needed - we use PlayerController
    }

    void OnFrame() {
        auto& c = Config::Get();

        if (!c.camerHack) {
            if (wasEnabled) {
                Restore();
                wasEnabled = false;
            }
            return;
        }

        if (!SDK::IsInGame()) return;

        wasEnabled = true;

        // ==========================================
        // Method 1: PlayerController CameraZoom
        // This is the most reliable method
        // CameraZoom is an offset from default (1134)
        // ==========================================
        auto ctrl = SDK::GetLocalPlayerController();
        if (ctrl && ctrl->IsValid()) {
            if (!savedOriginal) {
                originalZoom = ctrl->GetCameraZoom();
                savedOriginal = true;
            }

            float desiredOffset = c.cameraDistance - 1134.f;
            float currentZoom = ctrl->GetCameraZoom();

            if (std::abs(currentZoom - desiredOffset) > 1.f) {
                ctrl->SetCameraZoom(desiredOffset);
            }
        }

        // ==========================================
        // Method 2: Direct camera instance (if found)
        // ==========================================
        if (SDK::pCameraInstance) {
            auto currentDist = Memory::Read<float>(SDK::pCameraInstance);
            if (currentDist > 100.f && currentDist < 10000.f) {
                if (std::abs(currentDist - c.cameraDistance) > 1.f) {
                    Memory::Write<float>(SDK::pCameraInstance, c.cameraDistance);
                }
            }
        }

        // ==========================================
        // Method 3: ConVar "dota_camera_distance"
        // Try writing to the ConVar value directly
        // ==========================================
        // If pCameraInstance points to convar value:
        // Just write desired distance there
    }

    void Restore() {
        auto ctrl = SDK::GetLocalPlayerController();
        if (ctrl && ctrl->IsValid() && savedOriginal) {
            ctrl->SetCameraZoom(originalZoom);
        }

        if (SDK::pCameraInstance) {
            Memory::Write<float>(SDK::pCameraInstance, 1200.f);
        }

        savedOriginal = false;
    }
}
