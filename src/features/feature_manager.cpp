#include "feature_manager.h"
#include "camera_hack.h"
#include "esp.h"
#include "awareness.h"
#include "auto_accept.h"
#include "../config.h"

namespace Features {
    void Initialize() { CameraHack::Initialize(); }
    void OnFrame() {
        auto& c = Config::Get();
        if (c.camerHack) CameraHack::OnFrame();
        if (c.espEnabled) ESP::OnFrame();
        if (c.awarenessEnabled) Awareness::OnFrame();
        if (c.autoAccept) AutoAccept::OnFrame();
    }
    void Shutdown() { CameraHack::Restore(); }
}
