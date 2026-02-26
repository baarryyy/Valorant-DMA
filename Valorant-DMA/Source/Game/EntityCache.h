#pragma once
#include <vector>
#include "PlayerData.h"
#include "GameState.h"
#include "Offsets.h"
#include "../DMA/MemoryReader.h"
#include "../Misc/Types.h"

inline bool IsValidVA(uintptr_t address) {
    return address >= 0x10000 && address <= 0x7FFFFFFFFFFF;
}

class EntityCache {
public:
    EntityCache() = default;

    // Read all entity data via scatter reads. Returns player data + camera.
    // Pure data — no rendering, no aimbot.
    struct FrameData {
        std::vector<PlayerData> players;
        Camera camera;
        uintptr_t localPawn = 0;
    };

    FrameData Update(const MemoryReader& reader, const WorldSnapshot& snapshot);

private:
    // Pre-allocated to avoid per-frame heap allocations
    std::vector<uintptr_t> m_actorArray;
    std::vector<PlayerData> m_playersData;
};
