#include "feature_manager.h"
#include "camera_hack.h"
#include "esp.h"
#include "awareness.h"
#include "auto_accept.h"
#include "../config.h"
#include "../sdk/interfaces.h"

namespace Features {
    void Initialize() {
        CameraHack::Initialize();
    }

    void OnFrame() {
        // Always update SDK state first
        SDK::OnFrame();

        if (!SDK::IsInGame()) return;

        auto& c = Config::Get();
        if (c.camerHack)        CameraHack::OnFrame();
        if (c.espEnabled)       ESP::OnFrame();
        if (c.awarenessEnabled) Awareness::OnFrame();
        if (c.autoAccept)       AutoAccept::OnFrame();
    }

    void Shutdown() {
        CameraHack::Restore();
    }
}
