#pragma once
#include <cstdint>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include "../Misc/Types.h"
#include "../DMA/MemoryReader.h"
#include "Offsets.h"
#include "Decrypt.h"
#include "../Core/Logger.h"

// Thread-safe snapshot of the game's world state.
// Written by the game state thread, read by render/aimbot threads via atomic swap.
struct WorldSnapshot {
    uintptr_t uWorld = 0;
    uintptr_t gameState = 0;
    uintptr_t gameInstance = 0;
    uintptr_t persistentLevel = 0;
    uintptr_t localPlayers = 0;
    uintptr_t playerController = 0;
    uintptr_t localPawn = 0;
    uintptr_t playerCameraManager = 0;
    uintptr_t playerArray = 0;
    Camera localCamera = {};
    int playerCount = 0;
};

class GameState {
public:
    GameState() = default;
    ~GameState() { Stop(); }

    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;

    void Start(const MemoryReader& reader, uintptr_t baseAddress, bool debug = false);
    void Stop();

    // Get the latest snapshot (thread-safe, lock-free)
    std::shared_ptr<WorldSnapshot> GetSnapshot() const {
        return std::atomic_load(&m_snapshot);
    }

    bool IsRunning() const { return m_running.load(); }

private:
    void UpdateLoop(const MemoryReader& reader, uintptr_t baseAddress, bool debug);

    std::shared_ptr<WorldSnapshot> m_snapshot = std::make_shared<WorldSnapshot>();
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
};
