#include "Decrypt.h"
#include "Offsets.h"

#define HIDWORD(x) ((unsigned int)(((x) >> 32) & 0xFFFFFFFF))
#define __ROL8__(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define __ROR8__(x, r) (((x) >> (r)) | ((x) << (64 - (r))))

// Decryption function rewritten to avoid goto across variable initialization (MSVC C2362).
// Rewritten to avoid MSVC C2362 errors while preserving exact logic.
static __forceinline __int64 Decrypt_UWorld_Safe(const uint32_t key, const uintptr_t* state) {
    unsigned __int64 hash = 2685821657736338717i64
        * ((unsigned int)key ^ (unsigned int)(key << 25) ^ (((unsigned int)key ^ ((unsigned __int64)(unsigned int)key >> 15)) >> 12));

    unsigned __int64 stateIdx = hash % 7;
    unsigned __int64 value = state[stateIdx];
    unsigned __int64 highBits = hash >> 32;
    unsigned int branch = (unsigned int)stateIdx % 7;

    // Phase 1: initial transformation based on branch 0 or 1
    if (branch == 0) {
        unsigned __int64 t = value - (unsigned int)(highBits - 1);
        unsigned __int64 v23 = (2 * t) ^ ((2 * t) ^ (t >> 1)) & 0x5555555555555555i64;
        unsigned __int64 v24 = (4 * v23) ^ ((4 * v23) ^ (v23 >> 2)) & 0x3333333333333333i64;
        unsigned __int64 v25 = (16 * v24) ^ ((16 * v24) ^ (v24 >> 4)) & 0xF0F0F0F0F0F0F0Fi64;
        value = __ROL8__((v25 << 8) ^ ((v25 << 8) ^ (v25 >> 8)) & 0xFF00FF00FF00FFi64, 32);
    }
    else if (branch == 1) {
        int v26 = 2 * (int)stateIdx;
        value = __ROL8__(value - (unsigned int)(v26 + highBits),
                        (unsigned __int8)(((int)highBits + (int)stateIdx) % 0x3Fu) + 1);
    }

    // Phase 2: secondary transformations
    unsigned int combined = 2 * (unsigned int)stateIdx + (unsigned int)highBits;

    if (branch == 2)
        value = ~(value - combined);

    switch (branch) {
    case 3u: {
        __int64 v28 = 2 * ((2 * value) ^ ((2 * value) ^ (value >> 1)) & 0x5555555555555555i64);
        value = v28 ^ (v28 ^ (((2 * value) ^ ((2 * value) ^ (value >> 1)) & 0x5555555555555555i64) >> 1)) & 0x5555555555555555i64;
        break;
    }
    case 4u: {
        unsigned __int64 v29 = __ROR8__(value, (unsigned __int8)(combined % 0x3F) + 1);
        value = (2 * v29) ^ ((2 * v29) ^ (v29 >> 1)) & 0x5555555555555555i64;
        break;
    }
    case 5u: {
        unsigned __int64 v30 = __ROR8__(value, (unsigned __int8)(combined % 0x3F) + 1);
        unsigned __int64 v31 = (2 * v30) ^ ((2 * v30) ^ (v30 >> 1)) & 0x5555555555555555i64;
        unsigned __int64 v32 = (4 * v31) ^ ((4 * v31) ^ (v31 >> 2)) & 0x3333333333333333i64;
        unsigned __int64 v33 = (16 * v32) ^ ((16 * v32) ^ (v32 >> 4)) & 0xF0F0F0F0F0F0F0Fi64;
        value = __ROL8__((v33 << 8) ^ ((v33 << 8) ^ (v33 >> 8)) & 0xFF00FF00FF00FFi64, 32);
        break;
    }
    case 6u:
        value = ~value - (unsigned int)(highBits + stateIdx);
        break;
    }

    return value ^ (unsigned int)key;
}

uintptr_t DecryptUWorld(uintptr_t baseAddress, const MemoryReader& reader) {
    // Read key and state in sequence (could be batched with scatter for perf)
    auto key = reader.Read<uintptr_t>(baseAddress + Offsets::Decrypt::StateAddress + Offsets::Decrypt::KeyOffset);

#pragma pack(push, 1)
    struct DecryptState {
        uint64_t keys[7];
    };
#pragma pack(pop)

    auto state = reader.Read<DecryptState>(baseAddress + Offsets::Decrypt::StateAddress);
    auto decryptedPtr = Decrypt_UWorld_Safe(static_cast<uint32_t>(key), reinterpret_cast<uintptr_t*>(&state));
    return reader.Read<uintptr_t>(decryptedPtr);
}
