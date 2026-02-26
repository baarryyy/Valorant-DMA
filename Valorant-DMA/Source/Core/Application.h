#pragma once
#include "Config.h"
#include "Logger.h"
#include "../DMA/DMADevice.h"
#include "../DMA/ProcessContext.h"
#include "../DMA/MemoryReader.h"
#include "../DMA/KeyboardReader.h"
#include "../Game/GameState.h"
#include "../Game/EntityCache.h"
#include "../Combat/Aimbot.h"
#include "../Combat/TargetSelector.h"
#include "../Render/Overlay.h"
#include "../Render/ESP.h"
#include "../Render/Menu.h"
#include "../Render/Style.h"
#include "../Input/KMBoxSerial.h"
#include "../Input/KMBoxNet.h"
#include <memory>

class Application {
public:
    Application() = default;
    ~Application() { Shutdown(); }

    bool Init();
    void Run();
    void Shutdown();

private:
    Config m_config;
    DMADevice m_dma;
    ProcessContext m_process;
    MemoryReader m_reader;
    KeyboardReader m_keyboard;
    GameState m_gameState;
    EntityCache m_entityCache;
    Overlay m_overlay;
    Aimbot m_aimbot;

    std::unique_ptr<KMBoxSerial> m_kmboxSerial;
    std::unique_ptr<KMBoxNet> m_kmboxNet;
    IInputDevice* m_activeInput = nullptr;

    bool m_running = false;
};
