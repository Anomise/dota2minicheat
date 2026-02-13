#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <filesystem>

DWORD GetPID(const wchar_t* name) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (s == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    if (Process32FirstW(s, &pe)) { do { if (!wcscmp(pe.szExeFile, name)) { CloseHandle(s); return pe.th32ProcessID; } } while (Process32NextW(s, &pe)); }
    CloseHandle(s); return 0;
}

bool Inject(DWORD pid, const std::string& dll) {
    HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hp) return false;
    auto fp = std::filesystem::absolute(dll).string();
    void* rm = VirtualAllocEx(hp, nullptr, fp.size()+1, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!rm) { CloseHandle(hp); return false; }
    WriteProcessMemory(hp, rm, fp.c_str(), fp.size()+1, nullptr);
    HANDLE ht = CreateRemoteThread(hp, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA")),
        rm, 0, nullptr);
    if (!ht) { VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    WaitForSingleObject(ht, 10000);
    VirtualFreeEx(hp, rm, 0, MEM_RELEASE);
    CloseHandle(ht); CloseHandle(hp);
    return true;
}

int main() {
    SetConsoleTitleA("Dota 2 Injector");
    std::cout << "\n  DOTA 2 CHEAT INJECTOR v2.0\n\n";
    std::cout << "[*] Waiting for dota2.exe...\n";
    DWORD pid = 0;
    while (!(pid = GetPID(L"dota2.exe"))) { Sleep(1000); std::cout << "."; }
    std::cout << "\n[+] Found PID: " << pid << "\n[*] Loading...\n";
    Sleep(5000);
    std::string dll = "dota2minicheat.dll";
    if (!std::filesystem::exists(dll)) { std::cerr << "[-] DLL not found!\n"; std::cin.get(); return 1; }
    std::cout << "[*] Injecting...\n";
    if (Inject(pid, dll)) std::cout << "[+] Success! Press INSERT in-game.\n";
    else std::cerr << "[-] Failed!\n";
    std::cout << "\nPress Enter to close...\n"; std::cin.get();
    return 0;
}
