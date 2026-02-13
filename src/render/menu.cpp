static void TabSettings() {
    auto& c = Config::Get();
    Sec("Menu Settings");
    ImGui::SliderFloat("Opacity", &c.menuAlpha, 0.5f, 1, "%.2f");
    const char* ac[] = {"Crimson","Ocean","Emerald","Purple","Orange"};
    if (ImGui::Combo("Accent", &c.menuAccent, ac, IM_ARRAYSIZE(ac)))
        Style::ApplyModernDark(c.menuAlpha, c.menuAccent);
    Style::ToggleButton("Stream Proof", &c.streamProof);

    Sec("Config");
    if (ImGui::Button("Save", {-1, 36})) c.Save("dota2cheat.cfg");
    if (ImGui::Button("Load", {-1, 36})) { c.Load("dota2cheat.cfg"); Style::ApplyModernDark(c.menuAlpha, c.menuAccent); }
    if (ImGui::Button("Reset", {-1, 36})) { c = Config(); Style::ApplyModernDark(c.menuAlpha, c.menuAccent); }

    Sec("SDK Status");
    auto StatusLine = [](const char* name, uintptr_t ptr) {
        if (ptr)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "%s: 0x%llX", name, ptr);
        else
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "%s: NOT FOUND", name);
    };
    StatusLine("EntitySystem", SDK::pGameEntitySystem);
    StatusLine("LocalPlayer",  SDK::pLocalPlayer);
    StatusLine("ViewMatrix",   SDK::pViewMatrix);
    StatusLine("GameRules",    SDK::pGameRules);
    StatusLine("Camera",       SDK::pCameraInstance);
    ImGui::Separator();
    ImGui::Text("In Game: %s", SDK::IsInGame() ? "YES" : "NO");
    ImGui::Text("Max Entities: %d", SDK::GetMaxEntities());
    auto lh = SDK::GetLocalHero();
    if (lh) {
        ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Local Hero: %s", lh->GetUnitName());
    } else {
        ImGui::TextColored({1.f, 0.6f, 0.2f, 1.f}, "Local Hero: not found");
    }

    Sec("Info");
    ImGui::TextDisabled("Dota2 Cheat v2.0 | %s", __DATE__);
    ImGui::TextDisabled("INSERT=Menu END=Unload FPS:%.0f", ImGui::GetIO().Framerate);
}
