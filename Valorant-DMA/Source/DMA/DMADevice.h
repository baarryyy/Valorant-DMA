#pragma once
#include <Windows.h>
#include <vmmdll.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include "../Core/Logger.h"

class DMADevice {
public:
    DMADevice() = default;
    ~DMADevice();

    DMADevice(const DMADevice&) = delete;
    DMADevice& operator=(const DMADevice&) = delete;

    bool Init(bool useMemMap = true, bool debug = false);
    VMM_HANDLE GetHandle() const { return m_handle; }
    bool IsInitialized() const { return m_initialized; }

private:
    bool DumpMemoryMap(bool debug);
    bool ConfigureFPGA();

    VMM_HANDLE m_handle = nullptr;
    HMODULE m_vmm = nullptr;
    HMODULE m_ftd3xx = nullptr;
    HMODULE m_leechcore = nullptr;
    bool m_initialized = false;
};
