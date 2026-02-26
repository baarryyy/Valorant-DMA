#include "Application.h"

static constexpr const char* PROCESS_NAME = "Valorant-Win64-Shipping.exe";
static constexpr const char* CONFIG_FILE = "config.txt";

bool Application::Init() {
    Log::Init();
    LOG_INFO("Starting application...");

    // Load config
    m_config.Load(CONFIG_FILE);

    // Init DMA
    if (!m_dma.Init(true, false)) {
        LOG_ERROR("Failed to initialize DMA device");
        return false;
    }

    // Attach to process
    if (!m_process.Attach(m_dma.GetHandle(), PROCESS_NAME)) {
        LOG_ERROR("Failed to attach to {}", PROCESS_NAME);
        return false;
    }

    // Fix CR3 if needed
    m_process.FixCr3();

    // Init memory reader
    m_reader.Init(m_dma.GetHandle(), m_process.GetPID());

    // Init keyboard (non-fatal if it fails)
    if (!m_keyboard.Init(m_dma.GetHandle())) {
        LOG_WARN("Keyboard init failed - key detection will be unavailable");
    }

    // Init overlay
    if (!m_overlay.Init(m_config.width, m_config.height)) {
        LOG_ERROR("Failed to initialize overlay");
        return false;
    }
    ApplyStyle(m_config);

    // Init input devices
    m_kmboxSerial = std::make_unique<KMBoxSerial>();
    m_kmboxSerial->SetBaudRate(m_config.kmbox.baudrate);

    m_kmboxNet = std::make_unique<KMBoxNet>();
    m_kmboxNet->SetConnectionInfo(m_config.kmbox.ip, m_config.kmbox.port, m_config.kmbox.uuid);

    // Try connecting input devices
    if (m_kmboxNet->Connect()) {
        m_config.kmbox.useNet = true;
        m_config.kmbox.useSerial = false;
        m_activeInput = m_kmboxNet.get();
        LOG_INFO("KMBox .NET connected");
    }
    else if (m_kmboxSerial->Connect()) {
        m_config.kmbox.useSerial = true;
        m_config.kmbox.useNet = false;
        m_activeInput = m_kmboxSerial.get();
        LOG_INFO("KMBox B+ connected");
    }
    else {
        LOG_WARN("No KMBox device found - aimbot will be unavailable");
    }

    // Setup aimbot
    if (m_activeInput) {
        m_aimbot.SetInputDevice(m_activeInput);
        m_aimbot.SetConfig(&m_config);
    }

    // Start game state thread
    m_gameState.Start(m_reader, m_process.GetBaseAddress(), true);

    m_running = true;
    LOG_INFO("Application initialized successfully");
    return true;
}

void Application::Run() {
    while (m_running) {
        if (!m_overlay.ProcessMessages()) {
            m_running = false;
            break;
        }

        // Get latest game state snapshot
        auto snapshot = m_gameState.GetSnapshot();
        if (!snapshot || !snapshot->uWorld) {
            m_overlay.BeginFrame();
            Menu::Draw(m_config, m_kmboxSerial.get(), m_kmboxNet.get());
            m_overlay.EndFrame(m_config.misc.transparent);
            continue;
        }

        // Update entity cache
        auto frameData = m_entityCache.Update(m_reader, *snapshot);

        // Begin render frame
        m_overlay.BeginFrame();

        // Draw menu
        Menu::Draw(m_config, m_kmboxSerial.get(), m_kmboxNet.get());

        // Draw ESP
        ESP::Draw(frameData, m_config);

        // Aimbot logic
        if (m_config.aimbot.enable && m_activeInput) {
            bool aimKeyHeld = false;
            if (m_config.aimbot.aimKey1 > 0)
                aimKeyHeld |= m_keyboard.IsKeyDown(m_config.aimbot.aimKey1);
            if (m_config.aimbot.aimKey2 > 0)
                aimKeyHeld |= m_keyboard.IsKeyDown(m_config.aimbot.aimKey2);

            if (aimKeyHeld) {
                auto target = TargetSelector::FindBestTarget(frameData, m_config);
                if (target) {
                    m_aimbot.SetTarget(*target);
                    if (!m_aimbot.IsRunning())
                        m_aimbot.Start();
                }
                else {
                    m_aimbot.ClearTarget();
                }
            }
            else {
                m_aimbot.Stop();
            }
        }

        // End render frame
        m_overlay.EndFrame(m_config.misc.transparent);
    }
}

void Application::Shutdown() {
    if (!m_running) return;
    m_running = false;

    LOG_INFO("Shutting down...");

    m_aimbot.Stop();
    m_gameState.Stop();
    m_overlay.Shutdown();

    if (m_kmboxSerial) m_kmboxSerial->Disconnect();
    if (m_kmboxNet) m_kmboxNet->Disconnect();

    LOG_INFO("Shutdown complete");
}
