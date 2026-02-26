#pragma once
#include <vector>
#include <imgui/imgui.h>
#include "../Game/PlayerData.h"
#include "../Game/EntityCache.h"
#include "../Core/Config.h"
#include "Math.h"

namespace ESP {
    // Draw ESP for all players. Pure rendering — no DMA reads.
    inline void Draw(const EntityCache::FrameData& frame, const Config& config) {
        auto* drawList = ImGui::GetForegroundDrawList();
        int w = config.width;
        int h = config.height;

        for (const auto& player : frame.players) {
            if (!player.IsValid() || player.pawnPrivate == frame.localPawn)
                continue;
            if (!player.IsActive())
                continue;

            Vector3 head3D = BoneToWorld(player.headTransform, player.comp2World);
            Vector3 bottom3D = BoneToWorld(player.rootTransform, player.comp2World);

            auto head2D = WorldToScreen(frame.camera, head3D, w, h);
            auto bottom2D = WorldToScreen(frame.camera, bottom3D, w, h);

            if (!head2D || !bottom2D) continue;

            bool isVisible = player.IsVisible();
            ImColor boxColor = isVisible ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255);

            float boxHeight = bottom2D->y - head2D->y;
            float boxWidth = boxHeight * 0.5f;

            // Bounding box
            if (config.visuals.box) {
                drawList->AddRect(
                    ImVec2(head2D->x - boxWidth / 2, head2D->y),
                    ImVec2(head2D->x + boxWidth / 2, head2D->y + boxHeight),
                    boxColor, 3.0f, 0, 1.0f
                );
            }

            // Snap lines
            if (config.visuals.lines) {
                drawList->AddLine(
                    ImVec2((float)w / 2, (float)h),
                    ImVec2(bottom2D->x, bottom2D->y),
                    boxColor, 1.0f
                );
            }

            // Health bar
            if (config.visuals.health) {
                float healthPct = player.health / 100.0f;
                float barWidth = 5.0f;
                float barX = head2D->x + boxWidth / 2 + 2;

                // Background (red)
                drawList->AddRectFilled(
                    ImVec2(barX, head2D->y),
                    ImVec2(barX + barWidth, head2D->y + boxHeight),
                    ImColor(255, 0, 0, 255)
                );

                // Health fill
                int r = static_cast<int>((1.0f - healthPct) * 255);
                int g = static_cast<int>(healthPct * 255);
                drawList->AddRectFilled(
                    ImVec2(barX, head2D->y + boxHeight * (1.0f - healthPct)),
                    ImVec2(barX + barWidth, head2D->y + boxHeight),
                    ImColor(r, g, 0, 255)
                );
            }
        }
    }
}
