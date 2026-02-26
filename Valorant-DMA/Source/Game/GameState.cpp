#include "GameState.h"

void GameState::Start(const MemoryReader& reader, uintptr_t baseAddress, bool debug) {
    if (m_running.load()) return;
    m_running.store(true);
    m_thread = std::thread(&GameState::UpdateLoop, this, std::ref(reader), baseAddress, debug);
}

void GameState::Stop() {
    m_running.store(false);
    if (m_thread.joinable())
        m_thread.join();
}

void GameState::UpdateLoop(const MemoryReader& reader, uintptr_t baseAddress, bool debug) {
    while (m_running.load()) {
        auto snap = std::make_shared<WorldSnapshot>();

        snap->uWorld = DecryptUWorld(baseAddress, reader);
        if (!snap->uWorld) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (debug) LOG_DEBUG("UWorld: 0x{:X}", snap->uWorld);

        snap->gameState = reader.Read<uintptr_t>(snap->uWorld + Offsets::UWorld::GameState);
        snap->gameInstance = reader.Read<uintptr_t>(snap->uWorld + Offsets::UWorld::GameInstance);
        snap->persistentLevel = reader.Read<uintptr_t>(snap->uWorld + Offsets::UWorld::PersistentLevel);

        if (snap->gameInstance) {
            auto tempLocal = reader.Read<uintptr_t>(snap->gameInstance + Offsets::Player::LocalPlayers);
            snap->localPlayers = reader.Read<uintptr_t>(tempLocal);
        }

        if (snap->localPlayers) {
            snap->playerController = reader.Read<uintptr_t>(snap->localPlayers + Offsets::Player::PlayerController);
        }

        if (snap->playerController) {
            snap->localPawn = reader.Read<uintptr_t>(snap->playerController + Offsets::Player::LocalPawn);
            snap->playerCameraManager = reader.Read<uintptr_t>(snap->playerController + Offsets::Player::CameraManager);
        }

        if (snap->persistentLevel) {
            snap->playerCount = reader.Read<int>(snap->persistentLevel + Offsets::Level::PlayerCount);
        }

        if (debug) {
            LOG_DEBUG("GameState: 0x{:X}, Players: {}", snap->gameState, snap->playerCount);
        }

        // Atomically publish the new snapshot
        std::atomic_store(&m_snapshot, snap);

        // Adaptive poll interval: faster when in-game, slower when waiting
        int sleepMs = snap->localPawn ? 200 : 500;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }
}
