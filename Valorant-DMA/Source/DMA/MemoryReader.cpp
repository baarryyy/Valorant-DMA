#include "MemoryReader.h"

static const char* s_hexdigits =
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\001\002\003\004\005\006\007\010\011\000\000\000\000\000\000"
    "\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000";

static uint8_t GetByte(const char* hex) {
    return static_cast<uint8_t>((s_hexdigits[(unsigned char)hex[0]] << 4) | s_hexdigits[(unsigned char)hex[1]]);
}

std::optional<uint64_t> MemoryReader::FindSignature(const char* signature, uint64_t rangeStart, uint64_t rangeEnd, DWORD pid) const {
    if (!signature || signature[0] == '\0' || rangeStart >= rangeEnd)
        return std::nullopt;

    DWORD targetPid = (pid == 0) ? m_pid : pid;
    std::vector<uint8_t> buffer(rangeEnd - rangeStart);

    if (!VMMDLL_MemReadEx(m_handle, targetPid, rangeStart, buffer.data(), static_cast<DWORD>(buffer.size()), 0, VMMDLL_FLAG_NOCACHE))
        return std::nullopt;

    const char* pat = signature;
    uint64_t firstMatch = 0;

    for (uint64_t i = rangeStart; i < rangeEnd; i++) {
        if (*pat == '?' || buffer[i - rangeStart] == GetByte(pat)) {
            if (!firstMatch)
                firstMatch = i;
            if (!pat[2])
                return firstMatch;
            pat += (*pat == '?') ? 2 : 3;
        }
        else {
            pat = signature;
            firstMatch = 0;
        }
    }

    return std::nullopt;
}
