#pragma once
#include <optional>
#include <vector>
#include <cmath>
#include <limits>
#include "../Misc/Types.h"
#include "../Game/PlayerData.h"
#include "../Game/EntityCache.h"
#include "../Render/Math.h"
#include "../Core/Config.h"

namespace TargetSelector {
    // Selects the closest enemy head to screen center within FOV.
    // Returns the 2D screen position of the best target's head.
    inline std::optional<Vector2> FindBestTarget(
        const EntityCache::FrameData& frame,
        const Config& config)
    {
        float centerX = config.width / 2.0f;
        float centerY = config.height / 2.0f;
        float minDist = std::numeric_limits<float>::max();
        std::optional<Vector2> bestTarget;

        for (const auto& player : frame.players) {
            if (!player.IsValid() || player.pawnPrivate == frame.localPawn)
                continue;
            if (!player.IsActive())
                continue;

            Vector3 head3D = BoneToWorld(player.headTransform, player.comp2World);
            auto head2D = WorldToScreen(frame.camera, head3D, config.width, config.height);
            if (!head2D) continue;

            float dx = head2D->x - centerX;
            float dy = head2D->y - centerY;
            float screenDist = std::sqrt(dx * dx + dy * dy);

            // Skip if outside FOV
            if (screenDist > config.aimbot.fov) continue;

            // Skip if inside deadzone
            if (screenDist < config.aimbot.deadzone) continue;

            if (screenDist < minDist) {
                minDist = screenDist;
                bestTarget = *head2D;
            }
        }

        return bestTarget;
    }
}
