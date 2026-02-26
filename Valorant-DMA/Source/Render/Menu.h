#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <Windows.h>
#include <string>
#include <map>
#include "../Core/Config.h"
#include "../Input/InputDevice.h"

namespace Menu {

    // Key binding button helper
    inline std::string GetKeyName(int hexKey) {
        char keyName[256] = { 0 };
        switch (hexKey) {
        case VK_LBUTTON: return "Left Mouse";
        case VK_RBUTTON: return "Right Mouse";
        case VK_MBUTTON: return "Middle Mouse";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        default:
            return GetKeyNameTextA(MapVirtualKeyA(hexKey, MAPVK_VK_TO_VSC) << 16, keyName, sizeof(keyName))
                ? std::string(keyName) : "Unknown";
        }
    }

    inline void KeyButton(const std::string& name, const std::string& id, int width, int& key) {
        static std::map<std::string, bool> listeningMap;
        bool& listening = listeningMap[id];

        if (listening) {
            for (int i = 1; i <= 255; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    key = (i == VK_ESCAPE) ? 0 : i;
                    listening = false;
                    break;
                }
            }
        }

        std::string text = listening ? "Press any key..." : (key == 0 ? name : GetKeyName(key));
        ImGui::PushID(id.c_str());
        if (ImGui::Button(text.c_str(), ImVec2((float)width, 0)))
            listening = true;
        ImGui::PopID();
    }

    // Draw the aimbot settings tab
    inline void DrawAimbotTab(Config& config) {
        ImGui::Checkbox("Enable Aimbot", &config.aimbot.enable);
        if (config.aimbot.enable) {
            KeyButton("Hold Key 1", "aim1", 120, config.aimbot.aimKey1);
            ImGui::SameLine();
            KeyButton("Hold Key 2", "aim2", 120, config.aimbot.aimKey2);
            ImGui::SetNextItemWidth(250);
            ImGui::SliderFloat("Speed", &config.aimbot.smoothness, 1.0f, 20.0f, "%.0f");
            ImGui::SetNextItemWidth(250);
            ImGui::SliderFloat("##Fov", &config.aimbot.fov, 1.0f, 300.0f, "%.0f");
            ImGui::SameLine();
            ImGui::Checkbox("FOV", &config.aimbot.showFov);
        }
    }

    // Draw the visuals settings tab
    inline void DrawVisualsTab(Config& config) {
        ImGui::Checkbox("Enable Box", &config.visuals.box);
        ImGui::Checkbox("Enable Lines", &config.visuals.lines);
        ImGui::Checkbox("Enable Health", &config.visuals.health);
    }

    // Draw the misc settings tab
    inline void DrawMiscTab(Config& config, IInputDevice* serialDevice, IInputDevice* netDevice) {
        ImGui::Text("Insert to open/close menu");
        if (ImGui::Button("Save Config", { 120, 20 }))
            config.Save("config.txt");
        ImGui::Checkbox("Transparent", &config.misc.transparent);

        // KMBox connection popup
        static bool showPopup = false;
        static bool showSerial = false;
        static bool showNet = false;

        // Input buffers for ImGui (need char arrays)
        static char ipBuf[128] = "";
        static char portBuf[128] = "";
        static char uuidBuf[128] = "";

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y);
        ImGui::SetCursorPosX(10);
        if (ImGui::Button("Select KMBox type", { 140, 20 })) showPopup = true;

        if (showPopup) {
            ImGui::OpenPopup("KMBox Settings");
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 400) / 2, (io.DisplaySize.y - 300) / 2));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

            if (ImGui::BeginPopupModal("KMBox Settings", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("KMBox should connect on load.");
                ImGui::Dummy(ImVec2(0, 7.5f));

                if (ImGui::Button("KMBox B+", { 200, 20 })) showSerial = true;
                if (showSerial) {
                    ImGui::SetNextItemWidth(130);
                    ImGui::InputScalar("BaudRate", ImGuiDataType_S32, &config.kmbox.baudrate);
                    if (ImGui::Button("Connect to B+", { 200, 20 })) {
                        if (serialDevice && serialDevice->Connect()) {
                            config.kmbox.useSerial = true;
                        }
                    }
                }

                ImGui::Dummy(ImVec2(0, 7.5f));
                if (ImGui::Button("KMBox .NET", { 200, 20 })) showNet = true;
                if (showNet) {
                    ImGui::Dummy(ImVec2(0, 7.5f));
                    ImGui::SetNextItemWidth(170);
                    ImGui::InputText("IP", ipBuf, sizeof(ipBuf));
                    ImGui::Dummy(ImVec2(0, 7.5f));
                    ImGui::SetNextItemWidth(170);
                    ImGui::InputText("Port", portBuf, sizeof(portBuf));
                    ImGui::Dummy(ImVec2(0, 7.5f));
                    ImGui::SetNextItemWidth(170);
                    ImGui::InputText("UUID", uuidBuf, sizeof(uuidBuf));
                    ImGui::Dummy(ImVec2(0, 7.5f));

                    if (ImGui::Button("Connect to .NET", { 200, 20 })) {
                        config.kmbox.ip = ipBuf;
                        config.kmbox.port = portBuf;
                        config.kmbox.uuid = uuidBuf;
                        if (netDevice && netDevice->Connect()) {
                            config.kmbox.useNet = true;
                        }
                    }
                }

                ImGui::Dummy(ImVec2(0, 7.5f));
                if (ImGui::Button("Close", { 200, 20 })) {
                    showPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleVar();
                ImGui::EndPopup();
            }
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 130);
        if (ImGui::Button("Close", { 120, 20 }))
            PostQuitMessage(0);
    }

    // Main menu render function
    inline void Draw(Config& config, IInputDevice* serialDevice, IInputDevice* netDevice) {
        static int currentTab = -1;
        static bool showMenu = true;

        if (GetAsyncKeyState(VK_INSERT) & 1)
            showMenu = !showMenu;

        // FOV circle
        if (config.aimbot.showFov) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::GetForegroundDrawList()->AddCircle(
                ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                config.aimbot.fov,
                ImColor(1.0f, 1.0f, 1.0f, 1.0f), 100, 1.0f
            );
        }

        if (!showMenu) return;

        ImGui::SetNextWindowSize({ 620, 350 });
        ImGui::Begin("DMA Overlay", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

        if (ImGui::Button("Aimbot", { 195, 20 })) currentTab = 0;
        ImGui::SameLine();
        if (ImGui::Button("Visuals", { 195, 20 })) currentTab = 1;
        ImGui::SameLine();
        if (ImGui::Button("Misc", { 195, 20 })) currentTab = 2;

        switch (currentTab) {
        case 0: DrawAimbotTab(config); break;
        case 1: DrawVisualsTab(config); break;
        case 2: DrawMiscTab(config, serialDevice, netDevice); break;
        }

        ImGui::End();
    }
}
