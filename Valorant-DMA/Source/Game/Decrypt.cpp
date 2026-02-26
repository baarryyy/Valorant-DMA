#include "Decrypt.h"
#include "Offsets.h"

#define HIDWORD(x) ((unsigned int)(((x) >> 32) & 0xFFFFFFFF))
#define __ROL8__(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define __ROR8__(x, r) (((x) >> (r)) | ((x) << (64 - (r))))

// Internal decryption function matching Riot's obfuscation.
// Variable names kept close to decompiler output for easy comparison when updating.
static __forceinline __int64 Decrypt_UWorld_Internal(const uint32_t key, const uintptr_t* state) {
    unsigned __int64 v19 = 2685821657736338717i64
        * ((unsigned int)key ^ (unsigned int)(key << 25) ^ (((unsigned int)key ^ ((unsigned __int64)(unsigned int)key >> 15)) >> 12))
        % 7;

    unsigned __int64 v20 = state[v19];
    unsigned __int64 v21 = (2685821657736338717i64
        * ((unsigned int)key ^ (unsigned int)(key << 25) ^ (((unsigned int)key ^ ((unsigned __int64)(unsigned int)key >> 15)) >> 12))) >> 32;

    unsigned int v22 = (unsigned int)v19 % 7;
    if (!v22) {
        unsigned __int64 v23 = (2 * (v20 - (unsigned int)(v21 - 1))) ^ ((2 * (v20 - (unsigned int)(v21 - 1))) ^ ((v20 - (unsigned int)(v21 - 1)) >> 1)) & 0x5555555555555555i64;
        unsigned __int64 v24 = (4 * v23) ^ ((4 * v23) ^ (v23 >> 2)) & 0x3333333333333333i64;
        unsigned __int64 v25 = (16 * v24) ^ ((16 * v24) ^ (v24 >> 4)) & 0xF0F0F0F0F0F0F0Fi64;
        v20 = __ROL8__((v25 << 8) ^ ((v25 << 8) ^ (v25 >> 8)) & 0xFF00FF00FF00FFi64, 32);
        goto LABEL_26;
    }
    if (v22 != 1) {
    LABEL_26:
        int v26 = 2 * (int)v19;
        goto LABEL_27_with(v26, v21, v22, v20, v19, key);
    }
    {
        int v26 = 2 * (int)v19;
        v20 = __ROL8__(v20 - (unsigned int)(v26 + v21), (unsigned __int8)(((int)v21 + (int)v19) % 0x3Fu) + 1);
    LABEL_27_with:
        unsigned int v27 = v26 + (unsigned int)v21;
        if (v22 == 2)
            v20 = ~(v20 - v27);
        switch (v22) {
        case 3u: {
            __int64 v28 = 2 * ((2 * v20) ^ ((2 * v20) ^ (v20 >> 1)) & 0x5555555555555555i64);
            v20 = v28 ^ (v28 ^ (((2 * v20) ^ ((2 * v20) ^ (v20 >> 1)) & 0x5555555555555555i64) >> 1)) & 0x5555555555555555i64;
            break;
        }
        case 4u: {
            unsigned __int64 v29 = __ROR8__(v20, (unsigned __int8)(v27 % 0x3F) + 1);
            v20 = (2 * v29) ^ ((2 * v29) ^ (v29 >> 1)) & 0x5555555555555555i64;
            break;
        }
        case 5u: {
            unsigned __int64 v30 = __ROR8__(v20, (unsigned __int8)(v27 % 0x3F) + 1);
            unsigned __int64 v31 = (2 * v30) ^ ((2 * v30) ^ (v30 >> 1)) & 0x5555555555555555i64;
            unsigned __int64 v32 = (4 * v31) ^ ((4 * v31) ^ (v31 >> 2)) & 0x3333333333333333i64;
            unsigned __int64 v33 = (16 * v32) ^ ((16 * v32) ^ (v32 >> 4)) & 0xF0F0F0F0F0F0F0Fi64;
            v20 = __ROL8__((v33 << 8) ^ ((v33 << 8) ^ (v33 >> 8)) & 0xFF00FF00FF00FFi64, 32);
            break;
        }
        case 6u:
            v20 = ~v20 - (unsigned int)(v21 + v19);
            break;
        }
        return v20 ^ (unsigned int)key;
    }
}

// Workaround: The decompiled code uses goto across variable initialization.
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
