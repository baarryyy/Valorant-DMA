#pragma once
#include <Windows.h>
#include <vmmdll.h>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <string>
#include "Registry.h"
#include "../Game/Offsets.h"
#include "../Core/Logger.h"

class KeyboardReader {
public:
    KeyboardReader() = default;

    bool Init(VMM_HANDLE handle);
    bool IsKeyDown(uint32_t virtualKeyCode);

private:
    void UpdateKeys();

    VMM_HANDLE m_handle = nullptr;
    uint64_t m_gafAsyncKeyState = 0;
    uint8_t m_stateBitmap[64] = {};
    uint8_t m_prevStateBitmap[256 / 8] = {};
    int m_winlogonPid = 0;
    Registry m_registry;
    std::chrono::time_point<std::chrono::system_clock> m_lastUpdate = std::chrono::system_clock::now();
    std::mutex m_mutex;
};
