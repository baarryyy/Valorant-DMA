# Repository Analysis: Valorant-DMA

## Overview

C++ Windows application using Direct Memory Access (DMA) hardware (FPGA-based) to read Valorant's game memory from a separate machine. Provides a DirectX 11 overlay with aimbot and ESP features rendered via ImGui.

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++20 (MSVC v143, Visual Studio 2022) |
| Build System | MSBuild / `.sln` + `.vcxproj` |
| Target Platform | Windows x64 |
| DMA Library | MemProcFS (`vmmdll.h`, `leechcore.h`) via PCILeech/FPGA |
| Rendering | DirectX 11 + ImGui |
| Input Spoofing | KMBox B+ (serial) and KMBox .NET (network) |
| Logging | spdlog (bundled) |
| Config | nlohmann/json (bundled) |
| PCH | `Pch.h` (precompiled header) |

## Architecture

```
Main.cpp                    (entry point)
  |
  +-- Memory (DMA layer)    Source/Dma/
  |     Memory.cpp/h         - FPGA init, read/write/scatter, signature scan, CR3 fix
  |     InputManager.cpp/h   - Keyboard state via DMA (gafAsyncKeyState from win32kbase)
  |     Registry.cpp/h       - Registry reads via DMA
  |     Shellcode.cpp/h      - Code cave finder, shellcode injection stubs
  |
  +-- Core (game loop)      Source/Game/
  |     Core.cpp/h            - UWorld decryption + game state cache refresh (2s loop)
  |     structs.hpp           - Vector2/3, FTransform, Camera, offsets namespace, GameCache
  |     sdk.hpp               - World-to-screen, matrix math, bone transforms
  |
  +-- Entity system          Source/Game/Entity/
  |     Cache.cpp/h           - Entity loop: scatter-reads all players, draws ESP, picks aimbot target
  |     Combat/Aimbot.cpp/h   - Aimbot thread: smooth cursor movement via KMBox
  |
  +-- KMBox integration     Source/Kmbox/
  |     kmbox.hpp / manager.hpp  - KMBox B+ serial protocol
  |     kmboxNet.cpp/h           - KMBox .NET network protocol
  |     HidTable.h               - HID key code table
  |
  +-- Rendering             Source/Render/
        RenderMenu.hpp        - DX11 init, window creation, ImGui render loop, menu UI
        Config.hpp            - ImGui style/theme
        CustomUI.hpp          - Custom UI widgets (key binding buttons)
        Font.h                - Embedded font data
```

## Key Data Flow

1. **Startup** (`Main.cpp`): Loads config, initializes DMA via MemProcFS/FPGA, finds the game process, gets the base address, inits DirectX/ImGui overlay, connects KMBox, spawns game thread.

2. **Game thread** (`Core::InitGame`): Runs every 2 seconds in a detached thread. Decrypts UWorld (obfuscated pointer), then reads the UE object chain: UWorld -> GameState, GameInstance, PersistentLevel, LocalPlayers, PlayerController, LocalPawn, CameraManager.

3. **Render loop** (`Cache::EntityLoop`): Each frame uses scatter reads (batched DMA) in 4 passes:
   - Pass 1: Camera, player array pointer, player count
   - Pass 2: Per-actor PawnPrivate, Mesh, DamageHandler, Dormant
   - Pass 3: BoneArray, ComponentToWorld, Health, visibility timestamps
   - Pass 4: Head and Root bone transforms

4. **Rendering**: Computes world-to-screen projection per enemy, draws boxes/lines/health bars. Tracks closest head within FOV for aimbot targeting.

5. **Aimbot**: When aim key is held, a thread smoothly moves cursor toward target via KMBox hardware (B+ serial or .NET UDP).

## UWorld Decryption

`Decrypt_UWorld` (`Core.cpp:9-76`) is a reverse-engineered routine using a key + 7-element state array with bit rotations, XOR, and bit-permutation patterns. State address `0xAF30580` and key offset `+0x38` are version-specific.

## Game Offsets

All UE offsets in `offsets` namespace (`structs.hpp:177-208`) are hardcoded and version-locked:
- `GameInstance = 0x1A0`, `PersistentLevel = 0x38`
- `LocalPawn = 0x448`, `Mesh = 0x418`
- `BoneArray = 0x5C8`, `Health = 0x1B0`, `CameraCache = 0x1F90`

## Configuration

JSON-based config (`config.txt`) via `Settings.hpp` with sections for aimbot, KMBox, visuals, and misc settings.

## Code Patterns

- **Global singletons**: `Memory mem`, `Core InitCore`, `Cache Entity`, `Aimbot aim` as `inline` globals
- **Scatter reads**: Batches DMA reads across all players per frame for reduced PCIe round trips
- **CR3 fix**: `Memory::FixCr3()` works around EAC's DTB manipulation by iterating candidate DTBs
- **Precompiled header**: `Pch.h` pulls in all Windows/STL/DMA/logging headers

## Issues Found

1. **Double `ImGui::EndFrame()`**: `render_menu()` (`RenderMenu.hpp:300`) calls `EndFrame()`, then the render loop calls it again, causing ImGui assertion failures.

2. **Thread safety**: `GameCache` values are written by game thread and read by render thread without synchronization.

3. **Aimbot thread race**: `StartAimbotThread` has a TOCTOU race between checking and setting `aimbot_running`.

4. **Hardcoded offsets**: Version-locked to a specific Valorant build; any update breaks it.

5. **`strncpy` without null termination**: `loadSettings` IP/port/UUID copies don't guarantee null termination at buffer boundaries.

6. **`goto reinit`**: `Memory::Init` uses goto for retry logic, risking resource leaks.

7. **Missing scatter handle validation**: `EntityLoop` doesn't check if `CreateScatterHandle()` succeeded.

## File Statistics

- ~146,000 total lines across all `.h`/`.cpp` files (including vendored libraries)
- ~20 project-specific source files (excluding spdlog, imgui, json)
- 4 git commits total
