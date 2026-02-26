#pragma once
#include <Windows.h>
#include <vmmdll.h>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include "../Core/Logger.h"

class ProcessContext {
public:
    ProcessContext() = default;

    bool Attach(VMM_HANDLE handle, const std::string& processName, bool memMap = true);
    bool FixCr3();

    DWORD GetPID() const { return m_pid; }
    uintptr_t GetBaseAddress() const { return m_baseAddress; }
    size_t GetBaseSize() const { return m_baseSize; }
    VMM_HANDLE GetVMMHandle() const { return m_vmmHandle; }

    DWORD GetPidFromName(const std::string& name) const;
    std::vector<int> GetPidListFromName(const std::string& name) const;
    uintptr_t GetModuleBase(const std::string& moduleName) const;
    size_t GetModuleSize(const std::string& moduleName) const;

private:
    VMM_HANDLE m_vmmHandle = nullptr;
    std::string m_processName;
    DWORD m_pid = 0;
    uintptr_t m_baseAddress = 0;
    size_t m_baseSize = 0;
};
