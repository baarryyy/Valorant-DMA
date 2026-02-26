#include "DMADevice.h"

DMADevice::~DMADevice() {
    if (m_handle) {
        VMMDLL_Close(m_handle);
        m_handle = nullptr;
    }
    if (m_vmm) { FreeLibrary(m_vmm); m_vmm = nullptr; }
    if (m_ftd3xx) { FreeLibrary(m_ftd3xx); m_ftd3xx = nullptr; }
    if (m_leechcore) { FreeLibrary(m_leechcore); m_leechcore = nullptr; }
    m_initialized = false;
}

bool DMADevice::DumpMemoryMap(bool debug) {
    LPCSTR args[] = { "", "-device", "fpga://algo=0", "", "" };
    int argc = 3;
    if (debug) {
        args[argc++] = "-v";
        args[argc++] = "-printf";
    }

    VMM_HANDLE handle = VMMDLL_Initialize(argc, (LPSTR*)args);
    if (!handle) {
        LOG_ERROR("Failed to open VMM handle for memory map dump");
        return false;
    }

    PVMMDLL_MAP_PHYSMEM pPhysMemMap = nullptr;
    if (!VMMDLL_Map_GetPhysMem(handle, &pPhysMemMap)) {
        LOG_ERROR("Failed to get physical memory map");
        VMMDLL_Close(handle);
        return false;
    }

    if (pPhysMemMap->dwVersion != VMMDLL_MAP_PHYSMEM_VERSION) {
        LOG_ERROR("Invalid VMM Map Version");
        VMMDLL_MemFree(pPhysMemMap);
        VMMDLL_Close(handle);
        return false;
    }

    if (pPhysMemMap->cMap == 0) {
        LOG_ERROR("Physical memory map is empty");
        VMMDLL_MemFree(pPhysMemMap);
        VMMDLL_Close(handle);
        return false;
    }

    std::stringstream sb;
    for (DWORD i = 0; i < pPhysMemMap->cMap; i++) {
        sb << std::hex << pPhysMemMap->pMap[i].pa << " "
           << (pPhysMemMap->pMap[i].pa + pPhysMemMap->pMap[i].cb - 1) << std::endl;
    }

    auto temp_path = std::filesystem::temp_directory_path();
    std::ofstream nFile(temp_path.string() + "\\mmap.txt");
    nFile << sb.str();
    nFile.close();

    VMMDLL_MemFree(pPhysMemMap);
    LOG_INFO("Memory map dumped successfully");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    VMMDLL_Close(handle);
    return true;
}

static unsigned char s_fpgaAbortConfig[4] = { 0x10, 0x00, 0x10, 0x00 };

bool DMADevice::ConfigureFPGA() {
    ULONG64 qwID = 0, qwVersionMajor = 0, qwVersionMinor = 0;
    if (!VMMDLL_ConfigGet(m_handle, LC_OPT_FPGA_FPGA_ID, &qwID) &&
        VMMDLL_ConfigGet(m_handle, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) &&
        VMMDLL_ConfigGet(m_handle, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor)) {
        LOG_WARN("Failed to lookup FPGA device, attempting to proceed");
        return false;
    }

    if ((qwVersionMajor >= 4) && ((qwVersionMajor >= 5) || (qwVersionMinor >= 7))) {
        LC_CONFIG config = { .dwVersion = LC_CONFIG_VERSION, .szDevice = "existing" };
        HANDLE lcHandle = LcCreate(&config);
        if (!lcHandle) {
            LOG_ERROR("Failed to create LeechCore device handle");
            return false;
        }

        LcCommand(lcHandle, LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002, 4,
                  reinterpret_cast<PBYTE>(&s_fpgaAbortConfig), NULL, NULL);
        LOG_INFO("FPGA PCIe register auto-cleared");
        LcClose(lcHandle);
    }

    return true;
}

bool DMADevice::Init(bool useMemMap, bool debug) {
    if (m_initialized) return true;

    m_vmm = LoadLibraryA("vmm.dll");
    m_ftd3xx = LoadLibraryA("FTD3XX.dll");
    m_leechcore = LoadLibraryA("leechcore.dll");

    if (!m_vmm || !m_ftd3xx || !m_leechcore) {
        LOG_ERROR("Failed to load DMA libraries (vmm={}, ftd3xx={}, leechcore={})",
                  (void*)m_vmm, (void*)m_ftd3xx, (void*)m_leechcore);
        return false;
    }

    for (int attempt = 0; attempt < 2; attempt++) {
        LPCSTR args[] = { "", "-device", "fpga://algo=0", "", "", "", "" };
        DWORD argc = 3;
        if (debug) {
            args[argc++] = "-v";
            args[argc++] = "-printf";
        }

        std::string mmapPath;
        bool shouldUseMap = useMemMap && (attempt == 0);

        if (shouldUseMap) {
            auto temp_path = std::filesystem::temp_directory_path();
            mmapPath = temp_path.string() + "\\mmap.txt";

            if (!std::filesystem::exists(mmapPath)) {
                if (!DumpMemoryMap(debug)) {
                    LOG_WARN("Could not dump memory map, retrying without it");
                    continue;
                }
            }
            args[argc++] = "-memmap";
            args[argc++] = mmapPath.c_str();
        }

        m_handle = VMMDLL_Initialize(argc, (LPSTR*)args);
        if (m_handle) break;

        if (attempt == 0 && shouldUseMap) {
            LOG_WARN("VMM init failed with memory map, retrying without");
        }
    }

    if (!m_handle) {
        LOG_ERROR("Failed to initialize VMM handle");
        return false;
    }

    ULONG64 FPGA_ID = 0, DEVICE_ID = 0;
    VMMDLL_ConfigGet(m_handle, LC_OPT_FPGA_FPGA_ID, &FPGA_ID);
    VMMDLL_ConfigGet(m_handle, LC_OPT_FPGA_DEVICE_ID, &DEVICE_ID);
    LOG_INFO("DMA initialized (FPGA ID: {}, Device ID: {})", FPGA_ID, DEVICE_ID);

    if (!ConfigureFPGA()) {
        VMMDLL_Close(m_handle);
        m_handle = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}
