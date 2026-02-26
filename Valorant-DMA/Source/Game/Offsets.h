#pragma once
#include <cstdint>

// ============================================================================
// ALL OFFSETS IN ONE FILE - Update this file on each game patch
// ============================================================================

namespace Offsets {

    // UWorld chain
    namespace UWorld {
        constexpr uintptr_t GameInstance    = 0x1A0;  // UWorld -> UGameInstance*
        constexpr uintptr_t PersistentLevel = 0x38;   // UWorld -> ULevel*
        constexpr uintptr_t GameState       = 0x1A0;  // UWorld -> AGameStateBase*
    }

    // Game instance / local player
    namespace Player {
        constexpr uintptr_t LocalPlayers     = 0x40;  // UGameInstance -> TArray<ULocalPlayer*>
        constexpr uintptr_t PlayerController = 0x38;  // ULocalPlayer -> APlayerController*
        constexpr uintptr_t LocalPawn        = 0x448; // APlayerController -> APawn*
        constexpr uintptr_t CameraManager    = 0x460; // APlayerController -> APlayerCameraManager*
        constexpr uintptr_t CameraCache      = 0x1F90;// APlayerCameraManager -> FMinimalViewInfo
    }

    // Actor / Pawn
    namespace Actor {
        constexpr uintptr_t PawnPrivate   = 0x308;  // APlayerState -> APawn*
        constexpr uintptr_t PlayerState   = 0x3D8;  // APawn -> APlayerState*
        constexpr uintptr_t TeamId        = 0xF8;   // APlayerState -> TeamId
        constexpr uintptr_t Dormant       = 0x100;  // AActor -> bDormant
        constexpr uintptr_t Mesh          = 0x418;  // ACharacter -> USkeletalMeshComponent*
        constexpr uintptr_t Velocity      = 0x168;  // AActor -> FVector Velocity
        constexpr uintptr_t IsDBNO        = 0x93A;  // (BitField Index -> 4)
        constexpr uintptr_t IsDying       = 0x758;  // (BitField Index -> 4)
        constexpr uintptr_t TeamComp      = 0x610;
    }

    // Mesh / Bone
    namespace Mesh {
        constexpr uintptr_t ComponentToWorld       = 0x250;  // USceneComponent -> FTransform
        constexpr uintptr_t BoneArray              = 0x5C8;  // USkinnedMeshComponent -> TArray<FTransform>
        constexpr uintptr_t BoneCount              = 0x5E0;  // USkinnedMeshComponent -> int32
        constexpr uintptr_t LastSubmitTime         = 0x380;
        constexpr uintptr_t LastRenderTimeOnScreen = 0x384;
    }

    // Damage
    namespace Damage {
        constexpr uintptr_t DamageHandler = 0x9E8;  // AShooterCharacter -> UDamageableComponent*
        constexpr uintptr_t Health        = 0x1B0;  // UDamageableComponent -> float
    }

    // Level
    namespace Level {
        constexpr uintptr_t PlayerArray = 0xA0;   // ULevel -> TArray<AActor*>
        constexpr uintptr_t PlayerCount = 0xA8;   // ULevel -> int32 (count at +8 from array)
    }

    // Decrypt addresses (update when decrypt changes)
    namespace Decrypt {
        constexpr uintptr_t StateAddress = 0xAF30580;
        constexpr uintptr_t KeyOffset    = 0x38;     // Relative to StateAddress
    }

    // Bone indices
    namespace Bone {
        constexpr int Head = 8;
        constexpr int Root = 0;
    }

    // Windows-specific offsets (for DMA keyboard reading)
    namespace Win {
        constexpr uintptr_t SessionGlobalSlotsOffset = 0x3110;  // win32ksgd.sys offset
        constexpr uintptr_t AsyncKeyState_22631      = 0x36A8;  // Windows >= 22631.3810
        constexpr uintptr_t AsyncKeyState_Default     = 0x3690;  // Windows < 22631.3810
    }
}
