#include "globals.h"
#include "hooks/hooks.h"
#include "config.h"

static void MainThread(HMODULE hModule) {
    // Always allocate console for debugging
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    printf("=================================\n");
    printf("  Dota 2 Cheat v2.0 Loaded\n");
    printf("=================================\n");

    Config::Get().Load("dota2cheat.cfg");

    printf("[*] Waiting before hook init...\n");
    Sleep(3000);

    printf("[*] Initializing hooks...\n");
    if (!Hooks::Initialize()) {
        printf("[-] Hook initialization FAILED!\n");
        printf("[*] Press any key to unload...\n");
        Sleep(5000);
        FreeLibraryAndExitThread(hModule, 0);
        return;
    }

    printf("[+] Hooks initialized OK!\n");
    printf("[+] INSERT = toggle menu\n");
    printf("[+] END = unload\n");
    printf("[*] Waiting for game...\n");

    while (!G::bShouldUnload) {
        Sleep(100);
    }

    printf("[*] Unloading...\n");
    Config::Get().Save("dota2cheat.cfg");
    Hooks::Shutdown();
    Sleep(1000);

    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        G::hModule = hModule;
        auto t = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
