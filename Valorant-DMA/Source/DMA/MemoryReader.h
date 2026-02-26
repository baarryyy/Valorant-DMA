#pragma once
#include <Windows.h>
#include <vmmdll.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <optional>
#include <string>
#include "../Core/Logger.h"

// RAII wrapper for scatter handles
class ScatterHandle {
public:
    ScatterHandle(VMM_HANDLE vmmHandle, DWORD pid, DWORD flags = VMMDLL_FLAG_NOCACHE)
        : m_vmmHandle(vmmHandle), m_pid(pid), m_flags(flags) {
        m_handle = VMMDLL_Scatter_Initialize(vmmHandle, pid, flags);
    }

    ~ScatterHandle() {
        if (m_handle) VMMDLL_Scatter_CloseHandle(m_handle);
    }

    ScatterHandle(const ScatterHandle&) = delete;
    ScatterHandle& operator=(const ScatterHandle&) = delete;

    ScatterHandle(ScatterHandle&& other) noexcept
        : m_handle(other.m_handle), m_vmmHandle(other.m_vmmHandle)
        , m_pid(other.m_pid), m_flags(other.m_flags) {
        other.m_handle = nullptr;
    }

    VMMDLL_SCATTER_HANDLE Get() const { return m_handle; }
    bool IsValid() const { return m_handle != nullptr; }

    void PrepareRead(uint64_t address, void* buffer, size_t size) {
        if (m_handle)
            VMMDLL_Scatter_PrepareEx(m_handle, address, static_cast<DWORD>(size), static_cast<PBYTE>(buffer), NULL);
    }

    template <typename T>
    void PrepareRead(uint64_t address, T* buffer) {
        PrepareRead(address, reinterpret_cast<void*>(buffer), sizeof(T));
    }

    void PrepareWrite(uint64_t address, void* buffer, size_t size) {
        if (m_handle)
            VMMDLL_Scatter_PrepareWrite(m_handle, address, static_cast<PBYTE>(buffer), static_cast<DWORD>(size));
    }

    void Execute() {
        if (m_handle) {
            VMMDLL_Scatter_ExecuteRead(m_handle);
            VMMDLL_Scatter_Clear(m_handle, m_pid, m_flags);
        }
    }

    void ExecuteWrite() {
        if (m_handle) {
            VMMDLL_Scatter_Execute(m_handle);
            VMMDLL_Scatter_Clear(m_handle, m_pid, m_flags);
        }
    }

private:
    VMMDLL_SCATTER_HANDLE m_handle = nullptr;
    VMM_HANDLE m_vmmHandle;
    DWORD m_pid;
    DWORD m_flags;
};

class MemoryReader {
public:
    MemoryReader() = default;

    void Init(VMM_HANDLE handle, DWORD pid) {
        m_handle = handle;
        m_pid = pid;
    }

    VMM_HANDLE GetVMMHandle() const { return m_handle; }
    DWORD GetPID() const { return m_pid; }

    bool Read(uintptr_t address, void* buffer, size_t size) const {
        DWORD pid = IsKernelAddress(address) ? 4 : m_pid;
        DWORD readSize = 0;
        if (!VMMDLL_MemReadEx(m_handle, pid, address, static_cast<PBYTE>(buffer), static_cast<DWORD>(size), &readSize, VMMDLL_FLAG_NOCACHE))
            return false;
        return (readSize == size);
    }

    bool Read(uintptr_t address, void* buffer, size_t size, DWORD pid) const {
        DWORD readSize = 0;
        if (!VMMDLL_MemReadEx(m_handle, pid, address, static_cast<PBYTE>(buffer), static_cast<DWORD>(size), &readSize, VMMDLL_FLAG_NOCACHE))
            return false;
        return (readSize == size);
    }

    template <typename T>
    T Read(uintptr_t address) const {
        T buffer{};
        Read(address, &buffer, sizeof(T));
        return buffer;
    }

    template <typename T>
    T Read(uintptr_t address, DWORD pid) const {
        T buffer{};
        Read(address, &buffer, sizeof(T), pid);
        return buffer;
    }

    bool Write(uintptr_t address, void* buffer, size_t size) const {
        return VMMDLL_MemWrite(m_handle, m_pid, address, static_cast<PBYTE>(buffer), static_cast<DWORD>(size));
    }

    template <typename T>
    void Write(uintptr_t address, T value) {
        Write(address, &value, sizeof(T));
    }

    uint64_t ReadChain(uint64_t base, const std::vector<uint64_t>& offsets) const {
        uint64_t result = Read<uint64_t>(base + offsets.at(0));
        for (size_t i = 1; i < offsets.size(); i++)
            result = Read<uint64_t>(result + offsets.at(i));
        return result;
    }

    ScatterHandle CreateScatter(DWORD flags = VMMDLL_FLAG_NOCACHE) const {
        return ScatterHandle(m_handle, m_pid, flags);
    }

    ScatterHandle CreateScatter(DWORD pid, DWORD flags) const {
        return ScatterHandle(m_handle, pid, flags);
    }

    std::optional<uint64_t> FindSignature(const char* signature, uint64_t rangeStart, uint64_t rangeEnd, DWORD pid = 0) const;

private:
    static bool IsKernelAddress(uintptr_t ptr) {
        return (ptr & 0xFFF0000000000000) == 0xFFF0000000000000;
    }

    VMM_HANDLE m_handle = nullptr;
    DWORD m_pid = 0;
};
