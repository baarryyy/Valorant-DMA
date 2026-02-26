#pragma once
#include <Windows.h>
#include <vmmdll.h>
#include <string>
#include "../Core/Logger.h"

enum class RegistryType {
    None = REG_NONE,
    SZ = REG_SZ,
    ExpandSZ = REG_EXPAND_SZ,
    Binary = REG_BINARY,
    DWORD_Val = REG_DWORD,
    QWORD_Val = REG_QWORD,
};

class Registry {
public:
    Registry() = default;

    void Init(VMM_HANDLE handle) { m_handle = handle; }

    std::string QueryValue(const char* path, RegistryType type) const {
        if (!m_handle) return "";

        BYTE buffer[512] = {};
        DWORD regType = static_cast<DWORD>(type);
        DWORD size = sizeof(buffer);

        if (!VMMDLL_WinReg_QueryValueExU(m_handle, const_cast<LPSTR>(path), &regType, buffer, &size)) {
            LOG_WARN("Registry query failed for: {}", path);
            return "";
        }

        if (type == RegistryType::DWORD_Val) {
            DWORD dwordValue = *reinterpret_cast<DWORD*>(buffer);
            return std::to_string(dwordValue);
        }

        std::wstring wstr(reinterpret_cast<wchar_t*>(buffer));
        return std::string(wstr.begin(), wstr.end());
    }

private:
    VMM_HANDLE m_handle = nullptr;
};
