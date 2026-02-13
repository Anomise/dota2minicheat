#pragma once
#include "memory.h"
#include <string>

namespace PatternScan {
    inline uintptr_t FindInterface(const std::string& module, const std::string& pattern) {
        auto addr = Memory::FindPattern(module, pattern);
        if (!addr) return 0;
        return Memory::ResolveRelative(addr, 3, 7);
    }
}
