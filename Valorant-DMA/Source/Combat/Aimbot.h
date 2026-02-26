#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <algorithm>
#include "../Misc/Types.h"
#include "../Input/InputDevice.h"
#include "../Core/Config.h"

class Aimbot {
public:
    Aimbot() = default;
    ~Aimbot() { Stop(); }

    Aimbot(const Aimbot&) = delete;
    Aimbot& operator=(const Aimbot&) = delete;

    void SetInputDevice(IInputDevice* device) { m_device = device; }
    void SetConfig(const Config* config) { m_config = config; }

    // Set the current target position (thread-safe)
    void SetTarget(const Vector2& target);
    void ClearTarget();

    void Start();
    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    void AimbotLoop();
    void MoveToTarget(float targetX, float targetY);

    IInputDevice* m_device = nullptr;
    const Config* m_config = nullptr;

    std::atomic<bool> m_running{ false };
    std::thread m_thread;
    std::mutex m_targetMutex;
    Vector2 m_targetPos = { 0, 0 };
    bool m_hasTarget = false;
};
