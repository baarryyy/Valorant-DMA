#include "Aimbot.h"
#include <cmath>

void Aimbot::SetTarget(const Vector2& target) {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_targetPos = target;
    m_hasTarget = true;
}

void Aimbot::ClearTarget() {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_hasTarget = false;
}

void Aimbot::Start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
        return;  // Already running
    m_thread = std::thread(&Aimbot::AimbotLoop, this);
}

void Aimbot::Stop() {
    m_running.store(false);
    if (m_thread.joinable())
        m_thread.join();
}

void Aimbot::MoveToTarget(float targetX, float targetY) {
    if (!m_device || !m_config) return;

    float centerX = m_config->width / 2.0f;
    float centerY = m_config->height / 2.0f;

    float dx = targetX - centerX;
    float dy = targetY - centerY;

    // Exponential smoothing: smoothness=1 → instant, smoothness=20 → very slow
    float t = 1.0f / m_config->aimbot.smoothness;
    float moveX = dx * t;
    float moveY = dy * t;

    // Clamp to reasonable max delta (prevent extreme jumps)
    constexpr float maxDelta = 500.0f;
    moveX = std::clamp(moveX, -maxDelta, maxDelta);
    moveY = std::clamp(moveY, -maxDelta, maxDelta);

    // Only send if movement is significant
    if (std::abs(moveX) > 0.5f || std::abs(moveY) > 0.5f) {
        m_device->MoveMouse(static_cast<int>(moveX), static_cast<int>(moveY));
    }
}

void Aimbot::AimbotLoop() {
    while (m_running.load()) {
        Vector2 target;
        bool hasTarget;
        {
            std::lock_guard<std::mutex> lock(m_targetMutex);
            target = m_targetPos;
            hasTarget = m_hasTarget;
        }

        if (hasTarget) {
            MoveToTarget(target.x, target.y);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
