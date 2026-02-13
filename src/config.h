#pragma once
#include <Windows.h>
#include <string>

struct Config {
    bool  camerHack       = true;
    float cameraDistance   = 1800.f;
    float cameraFOV       = 0.f;
    bool  cameraFog       = true;

    bool  espEnabled      = true;
    bool  espHealth       = true;
    bool  espMana         = true;
    bool  espNames        = true;
    bool  espHeroIcons    = true;
    bool  espSpellTracker = true;
    bool  espItems        = false;
    bool  espIllusions    = true;
    float espFontSize     = 14.f;

    bool  awarenessEnabled  = true;
    bool  towerRange        = true;
    bool  expRange          = true;
    bool  attackRange       = true;
    bool  blinkRange        = true;
    bool  dangerIndicator   = true;
    bool  lastHitHelper     = true;
    float towerRangeColor[4] = {1.f, 0.3f, 0.3f, 0.3f};
    float expRangeColor[4]   = {0.3f, 1.f, 0.3f, 0.2f};

    bool  autoAccept     = true;
    bool  autoMute       = false;

    bool  weatherHack    = false;
    int   weatherType    = 0;
    bool  particleHack   = false;
    float menuAlpha      = 0.95f;
    int   menuAccent     = 0;

    bool  streamProof     = false;
    int   menuKey         = 0x2D; // VK_INSERT = 0x2D

    void Save(const std::string& path);
    void Load(const std::string& path);

    static Config& Get() {
        static Config instance;
        return instance;
    }
};
