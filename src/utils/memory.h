#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <Psapi.h>

namespace Memory {
    uintptr_t GetModuleBase(const std::string& moduleName);
    uintptr_t GetModuleSize(const std::string& moduleName);

    template<typename T>
    T Read(uintptr_t address) {
        if (IsBadReadPtr(reinterpret_cast<void*>(address), sizeof(T)))
            return T{};
        return *reinterpret_cast<T*>(address);
    }

    template<typename T>
    void Write(uintptr_t address, const T& value) {
        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<T*>(address) = value;
        VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtect, &oldProtect);
    }

    template<typename T>
    T ReadChain(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t addr = base;
        for (size_t i = 0; i < offsets.size() - 1; i++) {
            addr = Read<uintptr_t>(addr + offsets[i]);
            if (!addr) return T{};
        }
        return Read<T>(addr + offsets.back());
    }

    uintptr_t FindPattern(const std::string& module, const std::string& pattern);
    uintptr_t ResolveRelative(uintptr_t address, int offset, int instrSize);
}
