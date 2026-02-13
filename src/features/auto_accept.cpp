#include "auto_accept.h"
#include "../config.h"
#include "../utils/memory.h"
#include "../sdk/offsets.h"

namespace AutoAccept {
    static float lastCheck = 0.f;
    void OnFrame() {
        auto& c = Config::Get();
        if (!c.autoAccept) return;
        float now = GetTickCount64() / 1000.f;
        if (now - lastCheck < 0.5f) return;
        lastCheck = now;
        // TODO: Implement GC-based auto accept
    }
}
