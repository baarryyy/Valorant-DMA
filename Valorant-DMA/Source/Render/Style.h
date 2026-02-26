#pragma once
#include <imgui/imgui.h>
#include <algorithm>
#include "../Core/Config.h"

inline void ApplyStyle(const Config& config) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 5.0f;

    auto adjustColor = [](ImVec4 color, float factor) -> ImVec4 {
        color.x = std::clamp(color.x * factor, 0.0f, 1.0f);
        color.y = std::clamp(color.y * factor, 0.0f, 1.0f);
        color.z = std::clamp(color.z * factor, 0.0f, 1.0f);
        return color;
    };

    const auto& tc = config.misc.textColor;
    const auto& bg = config.misc.bgColor;

    style.Colors[ImGuiCol_Text] = ImVec4(tc[0], tc[1], tc[2], tc[3]);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(bg[0], bg[1], bg[2], bg[3]);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(bg[0], bg[1], bg[2], bg[3]);
    style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = adjustColor(style.Colors[ImGuiCol_FrameBg], 1.2f);
    style.Colors[ImGuiCol_FrameBgActive] = adjustColor(style.Colors[ImGuiCol_FrameBg], 0.8f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = adjustColor(style.Colors[ImGuiCol_TitleBg], 1.2f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
}
