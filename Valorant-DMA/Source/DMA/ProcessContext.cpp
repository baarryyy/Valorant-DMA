#include "ProcessContext.h"

bool ProcessContext::Attach(VMM_HANDLE handle, const std::string& processName, bool memMap) {
    m_vmmHandle = handle;
    m_processName = processName;

    m_pid = GetPidFromName(processName);
    if (!m_pid) {
        LOG_ERROR("Process '{}' not found", processName);
        return false;
    }

    m_baseAddress = GetModuleBase(processName);
    if (!m_baseAddress) {
        LOG_ERROR("Failed to get base address for '{}'", processName);
        return false;
    }

    m_baseSize = GetModuleSize(processName);
    if (!m_baseSize) {
        LOG_ERROR("Failed to get base size for '{}'", processName);
        return false;
    }

    LOG_INFO("Attached to {} (PID: {}, Base: 0x{:X}, Size: 0x{:X})",
             processName, m_pid, m_baseAddress, m_baseSize);
    return true;
}

DWORD ProcessContext::GetPidFromName(const std::string& name) const {
    DWORD pid = 0;
    VMMDLL_PidGetFromName(m_vmmHandle, (LPSTR)name.c_str(), &pid);
    return pid;
}

std::vector<int> ProcessContext::GetPidListFromName(const std::string& name) const {
    std::vector<int> list;
    PVMMDLL_PROCESS_INFORMATION processInfo = nullptr;
    DWORD totalProcesses = 0;

    if (!VMMDLL_ProcessGetInformationAll(m_vmmHandle, &processInfo, &totalProcesses))
        return list;

    for (DWORD i = 0; i < totalProcesses; i++) {
        if (strstr(processInfo[i].szNameLong, name.c_str()))
            list.push_back(processInfo[i].dwPID);
    }
    VMMDLL_MemFree(processInfo);
    return list;
}

uintptr_t ProcessContext::GetModuleBase(const std::string& moduleName) const {
    std::wstring wName(moduleName.begin(), moduleName.end());
    PVMMDLL_MAP_MODULEENTRY moduleInfo = nullptr;

    if (!VMMDLL_Map_GetModuleFromNameW(m_vmmHandle, m_pid,
            const_cast<LPWSTR>(wName.c_str()), &moduleInfo, VMMDLL_MODULE_FLAG_NORMAL)) {
        return 0;
    }

    return moduleInfo->vaBase;
}

size_t ProcessContext::GetModuleSize(const std::string& moduleName) const {
    std::wstring wName(moduleName.begin(), moduleName.end());
    PVMMDLL_MAP_MODULEENTRY moduleInfo = nullptr;

    if (!VMMDLL_Map_GetModuleFromNameW(m_vmmHandle, m_pid,
            const_cast<LPWSTR>(wName.c_str()), &moduleInfo, VMMDLL_MODULE_FLAG_NORMAL)) {
        return 0;
    }

    return moduleInfo->cbImageSize;
}

struct DtbInfo {
    uint32_t index;
    uint32_t processId;
    uint64_t dtb;
    uint64_t kernelAddr;
    std::string name;
};

bool ProcessContext::FixCr3() {
    PVMMDLL_MAP_MODULEENTRY moduleEntry = nullptr;
    if (VMMDLL_Map_GetModuleFromNameU(m_vmmHandle, m_pid,
            const_cast<LPSTR>(m_processName.c_str()), &moduleEntry, NULL)) {
        return true;  // CR3 already correct
    }

    if (!VMMDLL_InitializePlugins(m_vmmHandle)) {
        LOG_ERROR("Failed to initialize VMM plugins for CR3 fix");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Wait for procinfo to complete
    while (true) {
        BYTE bytes[4] = { 0 };
        DWORD bytesRead = 0;
        auto nt = VMMDLL_VfsReadW(m_vmmHandle,
            const_cast<LPWSTR>(L"\\misc\\procinfo\\progress_percent.txt"),
            bytes, 3, &bytesRead, 0);
        if (nt == VMMDLL_STATUS_SUCCESS && atoi(reinterpret_cast<LPSTR>(bytes)) == 100)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Read DTB info
    static uint64_t cbSize = 0x80000;
    auto bytes = std::make_unique<BYTE[]>(cbSize);
    DWORD bytesRead = 0;
    auto nt = VMMDLL_VfsReadW(m_vmmHandle,
        const_cast<LPWSTR>(L"\\misc\\procinfo\\dtb.txt"),
        bytes.get(), static_cast<DWORD>(cbSize - 1), &bytesRead, 0);

    if (nt != VMMDLL_STATUS_SUCCESS) {
        LOG_ERROR("Failed to read DTB info");
        return false;
    }

    std::vector<uint64_t> possibleDtbs;
    std::string lines(reinterpret_cast<char*>(bytes.get()));
    std::istringstream iss(lines);
    std::string line;

    while (std::getline(iss, line)) {
        DtbInfo info = {};
        std::istringstream lineSS(line);
        if (lineSS >> std::hex >> info.index >> std::dec >> info.processId
                   >> std::hex >> info.dtb >> info.kernelAddr >> info.name) {
            if (info.processId == 0)
                possibleDtbs.push_back(info.dtb);
            if (m_processName.find(info.name) != std::string::npos)
                possibleDtbs.push_back(info.dtb);
        }
    }

    for (auto dtb : possibleDtbs) {
        VMMDLL_ConfigSet(m_vmmHandle, VMMDLL_OPT_PROCESS_DTB | m_pid, dtb);
        if (VMMDLL_Map_GetModuleFromNameU(m_vmmHandle, m_pid,
                const_cast<LPSTR>(m_processName.c_str()), &moduleEntry, NULL)) {
            LOG_INFO("CR3/DTB patched successfully (DTB: 0x{:X})", dtb);
            return true;
        }
    }

    LOG_ERROR("Failed to fix CR3 - no valid DTB found");
    return false;
}
