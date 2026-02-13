#pragma once
#include "../globals.h"
namespace InputHook {
    inline bool IsKeyPressed(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
    inline bool IsKeyJustPressed(int vk) {
        static bool prev[256] = {};
        bool cur = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool jp = cur && !prev[vk]; prev[vk] = cur; return jp;
    }
}
