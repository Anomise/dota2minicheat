#include "memory.h"

namespace Memory {

    uintptr_t GetModuleBase(const std::string& moduleName) {
        HMODULE hMod = GetModuleHandleA(moduleName.c_str());
        return reinterpret_cast<uintptr_t>(hMod);
    }

    uintptr_t GetModuleSize(const std::string& moduleName) {
        HMODULE hMod = GetModuleHandleA(moduleName.c_str());
        if (!hMod) return 0;
        MODULEINFO info{};
        GetModuleInformation(GetCurrentProcess(), hMod, &info, sizeof(info));
        return info.SizeOfImage;
    }

    uintptr_t FindPattern(const std::string& module, const std::string& pattern) {
        auto base = GetModuleBase(module);
        auto size = GetModuleSize(module);
        if (!base || !size) return 0;

        std::vector<int> bytes;
        std::string pat = pattern;
        for (size_t i = 0; i < pat.size(); ) {
            if (pat[i] == ' ') { i++; continue; }
            if (pat[i] == '?') {
                bytes.push_back(-1);
                i += (i + 1 < pat.size() && pat[i + 1] == '?') ? 2 : 1;
            } else {
                bytes.push_back(std::stoi(pat.substr(i, 2), nullptr, 16));
                i += 2;
            }
        }

        auto data = reinterpret_cast<uint8_t*>(base);
        auto patSize = bytes.size();

        for (size_t i = 0; i < size - patSize; i++) {
            bool found = true;
            for (size_t j = 0; j < patSize; j++) {
                if (bytes[j] != -1 && data[i + j] != static_cast<uint8_t>(bytes[j])) {
                    found = false;
                    break;
                }
            }
            if (found)
                return base + i;
        }
        return 0;
    }

    uintptr_t ResolveRelative(uintptr_t address, int offset, int instrSize) {
        auto rel = Read<int32_t>(address + offset);
        return address + instrSize + rel;
    }
}
