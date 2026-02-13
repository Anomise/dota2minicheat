#include "globals.h"
#include "hooks/hooks.h"
#include "config.h"
#include <thread>

static void MainThread(HMODULE hModule) {
#ifdef _DEBUG
    AllocConsole(); FILE* f; freopen_s(&f, "CONOUT$", "w", stdout);
    printf("[+] Dota 2 Cheat loaded!\n");
#endif
    Config::Get().Load("dota2cheat.cfg");
    Sleep(2000);
    if (!Hooks::Initialize()) {
#ifdef _DEBUG
        printf("[-] Hook init failed!\n");
#endif
        FreeLibraryAndExitThread(hModule, 0); return;
    }
#ifdef _DEBUG
    printf("[+] Hooks OK! INSERT=menu END=unload\n");
#endif
    while (!G::bShouldUnload) Sleep(100);
    Config::Get().Save("dota2cheat.cfg");
    Hooks::Shutdown();
    Sleep(500);
#ifdef _DEBUG
    if (f) fclose(f); FreeConsole();
#endif
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        G::hModule = hModule;
        auto t = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hModule, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
