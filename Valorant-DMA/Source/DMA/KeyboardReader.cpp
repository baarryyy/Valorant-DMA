#include "KeyboardReader.h"

bool KeyboardReader::Init(VMM_HANDLE handle) {
    m_handle = handle;
    m_registry.Init(handle);

    std::string winBuild = m_registry.QueryValue(
        "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentBuild",
        RegistryType::SZ);
    if (winBuild.empty()) return false;
    int winVer = std::stoi(winBuild);

    std::string ubrStr = m_registry.QueryValue(
        "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\UBR",
        RegistryType::DWORD_Val);
    if (ubrStr.empty()) return false;
    int ubr = std::stoi(ubrStr);

    DWORD winlogonPid = 0;
    VMMDLL_PidGetFromName(m_handle, (LPSTR)"winlogon.exe", &winlogonPid);
    if (!winlogonPid) {
        LOG_ERROR("winlogon.exe not found");
        return false;
    }
    m_winlogonPid = winlogonPid;

    if (winVer > 22000) {
        // Modern Windows 11 path via csrss.exe
        PVMMDLL_PROCESS_INFORMATION processInfo = nullptr;
        DWORD totalProcesses = 0;
        if (!VMMDLL_ProcessGetInformationAll(m_handle, &processInfo, &totalProcesses))
            return false;

        std::vector<int> csrssPids;
        for (DWORD i = 0; i < totalProcesses; i++) {
            if (strstr(processInfo[i].szNameLong, "csrss.exe"))
                csrssPids.push_back(processInfo[i].dwPID);
        }
        VMMDLL_MemFree(processInfo);

        for (auto pid : csrssPids) {
            uintptr_t tmp = VMMDLL_ProcessGetModuleBaseU(m_handle, pid, (LPSTR)"win32ksgd.sys");
            uintptr_t gSessionGlobalSlots = tmp + Offsets::Win::SessionGlobalSlotsOffset;
            uintptr_t userSessionState = 0;

            for (int i = 0; i < 4; i++) {
                uintptr_t slot = 0;
                VMMDLL_MemReadEx(m_handle, pid, gSessionGlobalSlots, (PBYTE)&slot, sizeof(slot), NULL, VMMDLL_FLAG_NOCACHE);
                uintptr_t slot2 = 0;
                VMMDLL_MemReadEx(m_handle, pid, slot + 8 * i, (PBYTE)&slot2, sizeof(slot2), NULL, VMMDLL_FLAG_NOCACHE);
                VMMDLL_MemReadEx(m_handle, pid, slot2, (PBYTE)&userSessionState, sizeof(userSessionState), NULL, VMMDLL_FLAG_NOCACHE);

                if (userSessionState > 0x7FFFFFFFFFFF)
                    break;
            }

            if (winVer >= 22631 && ubr >= 3810)
                m_gafAsyncKeyState = userSessionState + Offsets::Win::AsyncKeyState_22631;
            else
                m_gafAsyncKeyState = userSessionState + Offsets::Win::AsyncKeyState_Default;

            if (m_gafAsyncKeyState > 0x7FFFFFFFFFFF)
                break;
        }

        if (m_gafAsyncKeyState > 0x7FFFFFFFFFFF) {
            LOG_INFO("gafAsyncKeyState found at: 0x{:X}", m_gafAsyncKeyState);
            return true;
        }
        return false;
    }
    else {
        // Legacy Windows path via EAT
        PVMMDLL_MAP_EAT eatMap = nullptr;
        if (!VMMDLL_Map_GetEATU(m_handle,
                m_winlogonPid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY,
                (LPSTR)"win32kbase.sys", &eatMap)) {
            LOG_ERROR("Failed to get EAT for win32kbase.sys");
            return false;
        }

        if (eatMap->dwVersion != VMMDLL_MAP_EAT_VERSION) {
            VMMDLL_MemFree(eatMap);
            return false;
        }

        for (DWORD i = 0; i < eatMap->cMap; i++) {
            if (strcmp(eatMap->pMap[i].uszFunction, "gafAsyncKeyState") == 0) {
                m_gafAsyncKeyState = eatMap->pMap[i].vaFunction;
                break;
            }
        }
        VMMDLL_MemFree(eatMap);

        if (m_gafAsyncKeyState > 0x7FFFFFFFFFFF) {
            LOG_INFO("gafAsyncKeyState found at: 0x{:X}", m_gafAsyncKeyState);
            return true;
        }
        return false;
    }
}

void KeyboardReader::UpdateKeys() {
    uint8_t prevBitmap[64] = {};
    memcpy(prevBitmap, m_stateBitmap, 64);

    VMMDLL_MemReadEx(m_handle,
        m_winlogonPid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY,
        m_gafAsyncKeyState,
        reinterpret_cast<PBYTE>(&m_stateBitmap), 64, NULL, VMMDLL_FLAG_NOCACHE);

    for (int vk = 0; vk < 256; ++vk) {
        int byteIdx = (vk * 2) / 8;
        int bitPos = ((vk % 4) * 2);  // Explicit parentheses for clarity

        if ((m_stateBitmap[byteIdx] & (1 << bitPos)) &&
            !(prevBitmap[byteIdx] & (1 << bitPos))) {
            m_prevStateBitmap[vk / 8] |= (1 << (vk % 8));
        }
    }
}

bool KeyboardReader::IsKeyDown(uint32_t virtualKeyCode) {
    if (m_gafAsyncKeyState < 0x7FFFFFFFFFFF)
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    if (now - m_lastUpdate > std::chrono::milliseconds(1)) {
        UpdateKeys();
        m_lastUpdate = now;
    }

    int byteIdx = (virtualKeyCode * 2) / 8;
    int bitPos = ((virtualKeyCode % 4) * 2);
    return m_stateBitmap[byteIdx] & (1 << bitPos);
}
