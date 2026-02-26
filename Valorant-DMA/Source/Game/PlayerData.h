#pragma once
#include <cstdint>
#include "../Misc/Types.h"

struct PlayerData {
    uintptr_t pawnPrivate = 0;
    uintptr_t mesh = 0;
    uintptr_t boneArray = 0;
    uintptr_t boneArrayCache = 0;
    uintptr_t damageHandler = 0;
    float health = 0.0f;
    FTransform comp2World = {};
    FTransform headTransform = {};
    FTransform rootTransform = {};
    uint8_t dormant = 0;
    float lastSubmitTime = 0.0f;
    float lastRenderTime = 0.0f;

    bool IsVisible() const {
        return lastRenderTime + 0.06f >= lastSubmitTime;
    }

    bool IsValid() const {
        return pawnPrivate != 0 && mesh != 0 && boneArray != 0 && health > 0.0f;
    }

    bool IsActive() const {
        return !dormant || dormant == 1;
    }
};
