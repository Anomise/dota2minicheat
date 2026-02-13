#include "globals.h"
#include "hooks/hooks.h"
#include "config.h"
#include "utils/logger.h"

static void MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    LOG_INFO("=================================");
    LOG_INFO("  Dota 2 Cheat v2.0 Loaded");
    LOG_INFO("=================================");

    Config::Get().Load("dota2cheat.cfg");
    LOG_INFO("Config loaded");

    LOG_INFO("Waiting 3s before hook init...");
    Sleep(3000);

    LOG_INFO("Initializing hooks...");
    if (!Hooks::Initialize()) {
        LOG_ERROR("Hook initialization FAILED!");
        LOG_INFO("Unloading in 5s...");
        Sleep(5000);
        FreeLibraryAndExitThread(hModule, 0);
        return;
    }

    LOG_SUCCESS("Hooks initialized!");
    LOG_INFO("INSERT = toggle menu | END = unload");

    while (!G::bShouldUnload) Sleep(100);

    LOG_INFO("Unloading...");
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
