# Valorant DMA

A DMA-based overlay tool for Valorant using FPGA hardware, built with DirectX 11 and ImGui.

## Requirements

### Hardware
- **FPGA DMA card** (e.g. Squirrel, Enigma X1, etc.) connected to the target PC
- **KMBox B+** (serial) or **KMBox .NET** (network) for mouse input — optional, required for aimbot

### Software (Build Machine)
- **Visual Studio 2022** (v143 toolset)
- **Windows 10 SDK**
- **C++20** enabled (configured in the project)

### Runtime DLLs
Place these in the same directory as the compiled `.exe`:

| File | Source |
|------|--------|
| `vmm.dll` | [MemProcFS](https://github.com/ufrisk/MemProcFS/releases) |
| `leechcore.dll` | Included with MemProcFS |
| `FTD3XX.dll` | [FTDI D3XX drivers](https://ftdichip.com/drivers/d3xx-drivers/) |
| `info.db` | Included with MemProcFS |

### Link Libraries
Place these in the `Valorant-DMA/Libs/` folder before building:

| File | Source |
|------|--------|
| `vmmdll.lib` | MemProcFS release |
| `leechcore.lib` | MemProcFS release |
| `FTD3XX.lib` | FTDI D3XX SDK |

## Building

1. Open `Valorant-DMA.sln` in Visual Studio 2022
2. Ensure the `.lib` files are in `Valorant-DMA/Libs/`
3. Select **Release | x64**
4. Build (Ctrl+Shift+B)

The output binary will be in `x64/Release/`.

## Setup

### First Run

1. Connect your FPGA DMA card to the target machine
2. Launch Valorant on the target machine
3. Copy the runtime DLLs (`vmm.dll`, `leechcore.dll`, `FTD3XX.dll`, `info.db`) next to the built `.exe`
4. Run the `.exe` **on the second PC** (the one with the FPGA card)

On first launch the tool will:
- Dump the physical memory map to `%TEMP%\mmap.txt` (takes a few seconds)
- Initialize the FPGA device and configure PCIe registers
- Attach to `Valorant-Win64-Shipping.exe`
- Fix the CR3/DTB if needed (bypasses page table protections)
- Attempt to connect to a KMBox device (tries .NET first, then serial)
- Open the DirectX 11 overlay window

### KMBox Setup

**KMBox B+** (serial USB):
- Plug the KMBox B+ into the second PC via USB
- The tool auto-detects the `USB-SERIAL CH340` COM port
- Default baud rate: `115200` (configurable in the menu)

**KMBox .NET** (network):
- Connect the KMBox .NET to your local network
- Enter the device's IP, port, and UUID in the menu under **Misc > Select KMBox type**
- Click "Connect to .NET"

If no KMBox is connected, ESP visuals still work — only the aimbot requires a KMBox.

## Usage

### Menu Controls

Press **INSERT** to toggle the menu overlay.

The menu has three tabs:

#### Aimbot
| Setting | Description | Range |
|---------|-------------|-------|
| Enable Aimbot | Master toggle | on/off |
| Hold Key 1 / 2 | Aim activation keys (hold to aim) | any key |
| Speed | Smoothing factor (lower = faster snap) | 1 - 20 |
| FOV | Targeting field of view in pixels | 1 - 300 |
| FOV checkbox | Draw FOV circle on screen | on/off |

#### Visuals
| Setting | Description |
|---------|-------------|
| Enable Box | Bounding boxes around players (green = visible, red = not) |
| Enable Lines | Snap lines from screen bottom to player feet |
| Enable Health | Health bar next to each player box |

#### Misc
| Setting | Description |
|---------|-------------|
| Save Config | Saves all settings to `config.txt` |
| Transparent | Makes the overlay background transparent |
| Select KMBox type | Opens KMBox connection popup |
| Close | Exit the application |

### Config File

Settings are saved to `config.txt` (JSON) in the working directory. Example:

```json
{
    "width": 1920,
    "height": 1080,
    "aimbot": {
        "enable": false,
        "smoothness": 5.0,
        "deadzone": 0.0,
        "fov": 100.0,
        "showFov": false,
        "aimKey1": 0,
        "aimKey2": 0
    },
    "kmbox": {
        "useSerial": false,
        "useNet": false,
        "baudrate": 115200,
        "ip": "",
        "port": "",
        "uuid": ""
    },
    "visuals": {
        "box": false,
        "lines": false,
        "health": false
    },
    "misc": {
        "transparent": false
    }
}
```

Set `width` and `height` to match the **target machine's** screen resolution before launching.

## Architecture

```
Source/
├── Core/           Application, Config, Logger
├── DMA/            DMADevice, ProcessContext, MemoryReader, KeyboardReader, Registry
├── Game/           Offsets, Decrypt, GameState, EntityCache, PlayerData
├── Combat/         Aimbot, TargetSelector
├── Render/         Overlay (DX11), ESP, Menu (ImGui), Math, Style, Font
├── Input/          IInputDevice, KMBoxSerial, KMBoxNet
└── Misc/           Pch (precompiled header), Types (Vector2/3, FTransform, Camera)
```

### Threading Model

| Thread | Responsibility |
|--------|---------------|
| **Main** | Overlay rendering, ImGui menu, ESP drawing, aimbot key checks |
| **GameState** | Polls UWorld chain via DMA reads, publishes snapshots via atomic swap |
| **Aimbot** | Consumes target position, sends smoothed mouse deltas to KMBox |

The GameState thread writes `WorldSnapshot` objects and publishes them atomically. The main thread reads the latest snapshot lock-free via `std::atomic_load<shared_ptr>`. The aimbot thread receives target coordinates through a mutex-protected `Vector2`.

### Data Flow

```
DMA Card ──► MemoryReader ──► GameState Thread ──► WorldSnapshot (atomic)
                                                        │
                                              Main Thread reads
                                                        │
                                    ┌───────────────────┼────────────────┐
                                    ▼                   ▼                ▼
                              EntityCache          TargetSelector      Menu
                              (scatter reads)      (find best head)   (ImGui)
                                    │                   │
                                    ▼                   ▼
                                  ESP              Aimbot Thread
                               (draw boxes)      (move via KMBox)
```

## Updating Offsets

When Valorant updates, offsets may change. All offsets are centralized in a single file:

**`Source/Game/Offsets.h`**

Key offset groups:
- `UWorld` — World pointer chain
- `Player` — Local player / controller chain
- `Actor` — Pawn, player state, team, mesh
- `Mesh` — Bone array, component-to-world transform
- `Damage` — Health values
- `Level` — Actor array and count
- `Decrypt` — UWorld decryption state address and key offset

The decryption algorithm in `Source/Game/Decrypt.cpp` may also need updating when Riot changes their obfuscation routine.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Failed to load DMA libraries" | Ensure `vmm.dll`, `leechcore.dll`, `FTD3XX.dll` are next to the `.exe` |
| "Failed to initialize DMA device" | Check FPGA card connection, USB cable, and drivers |
| "Failed to attach to process" | Make sure Valorant is running on the target machine |
| "CR3/DTB fix failed" | Game may need a restart, or the FPGA firmware may need updating |
| "Keyboard init failed" | Non-fatal — DMA keyboard reading needs `winlogon.exe` access. Aim keys won't work but ESP still functions |
| "No KMBox device found" | Non-fatal — ESP works without KMBox, only aimbot is disabled |
| Overlay is a black window | Set `transparent: true` in config or toggle it in the Misc tab |
| Wrong resolution | Edit `width`/`height` in `config.txt` to match the target monitor |
| ESP positions are wrong | Offsets are outdated — update `Offsets.h` for the current game patch |

## License

This project is licensed under the MIT License. See the LICENSE file for details.
