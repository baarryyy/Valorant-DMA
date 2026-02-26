#pragma once
#include <string>
#include <fstream>
#include <algorithm>
#include <json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

struct AimbotConfig {
    bool enable = false;
    float smoothness = 5.0f;
    float deadzone = 0.0f;
    float fov = 100.0f;
    bool showFov = false;
    int aimKey1 = 0;
    int aimKey2 = 0;
};

struct KMBoxConfig {
    bool useSerial = false;
    bool useNet = false;
    int baudrate = 115200;
    std::string ip;
    std::string port;
    std::string uuid;
};

struct VisualsConfig {
    bool box = false;
    bool lines = false;
    bool health = false;
};

struct MiscConfig {
    bool transparent = false;
    int currentFps = 0;
    bool customColor = false;
    float accentColor[4] = { 0.28f, 0.56f, 1.00f, 1.00f };
    float textColor[4]   = { 0.95f, 0.96f, 0.98f, 1.00f };
    float bgColor[4]     = { 0.10f, 0.11f, 0.12f, 0.90f };
};

struct Config {
    int width = 1920;
    int height = 1080;
    AimbotConfig aimbot;
    KMBoxConfig kmbox;
    VisualsConfig visuals;
    MiscConfig misc;

    bool Save(const std::string& filename) const {
        try {
            json j;
            j["width"] = width;
            j["height"] = height;

            j["aimbot"]["enable"] = aimbot.enable;
            j["aimbot"]["smoothness"] = std::clamp(aimbot.smoothness, 1.0f, 20.0f);
            j["aimbot"]["deadzone"] = aimbot.deadzone;
            j["aimbot"]["fov"] = std::clamp(aimbot.fov, 1.0f, 300.0f);
            j["aimbot"]["showFov"] = aimbot.showFov;
            j["aimbot"]["aimKey1"] = aimbot.aimKey1;
            j["aimbot"]["aimKey2"] = aimbot.aimKey2;

            j["kmbox"]["useSerial"] = kmbox.useSerial;
            j["kmbox"]["useNet"] = kmbox.useNet;
            j["kmbox"]["baudrate"] = kmbox.baudrate;
            j["kmbox"]["ip"] = kmbox.ip;
            j["kmbox"]["port"] = kmbox.port;
            j["kmbox"]["uuid"] = kmbox.uuid;

            j["visuals"]["box"] = visuals.box;
            j["visuals"]["lines"] = visuals.lines;
            j["visuals"]["health"] = visuals.health;

            j["misc"]["transparent"] = misc.transparent;

            std::ofstream file(filename);
            if (!file.is_open()) return false;
            file << j.dump(4);
            return true;
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to save config: {}", e.what());
            return false;
        }
    }

    bool Load(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) return false;

            json j;
            file >> j;

            if (j.contains("width")) width = j["width"].get<int>();
            if (j.contains("height")) height = j["height"].get<int>();

            if (j.contains("aimbot")) {
                auto& a = j["aimbot"];
                aimbot.enable     = a.value("enable", false);
                aimbot.smoothness = std::clamp(a.value("smoothness", 5.0f), 1.0f, 20.0f);
                aimbot.deadzone   = a.value("deadzone", 0.0f);
                aimbot.fov        = std::clamp(a.value("fov", 100.0f), 1.0f, 300.0f);
                aimbot.showFov    = a.value("showFov", false);
                aimbot.aimKey1    = a.value("aimKey1", 0);
                aimbot.aimKey2    = a.value("aimKey2", 0);
            }

            if (j.contains("kmbox")) {
                auto& k = j["kmbox"];
                kmbox.useSerial = k.value("useSerial", false);
                kmbox.useNet    = k.value("useNet", false);
                kmbox.baudrate  = k.value("baudrate", 115200);
                kmbox.ip        = k.value("ip", std::string(""));
                kmbox.port      = k.value("port", std::string(""));
                kmbox.uuid      = k.value("uuid", std::string(""));
            }

            if (j.contains("visuals")) {
                auto& v = j["visuals"];
                visuals.box    = v.value("box", false);
                visuals.lines  = v.value("lines", false);
                visuals.health = v.value("health", false);
            }

            if (j.contains("misc")) {
                misc.transparent = j["misc"].value("transparent", false);
            }

            return true;
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to load config: {}", e.what());
            return false;
        }
    }
};
