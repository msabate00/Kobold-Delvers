<img width="720" height="467" alt="Kobold-Delvers Logo" src="https://github.com/user-attachments/assets/2f45c878-d1c7-4354-85b3-a4218b45e651" />

**SandForge** is a lightweight **falling-sand simulation engine** written in **C++17** with **OpenGL 3.3 Core**.
It provides a grid-based material simulation (cellular automata style), chunk-based updates, GPU rendering, basic UI/input, audio, and level I/O.

## What this repository is (and is not)

This is **not** an editor-driven engine like Unity where you “download and run the engine”.
SandForge is delivered as **source code**: you **fork/clone** the repository, open it in **Visual Studio (CMake)**, and build it.

- The CMake target builds a single executable: **`SandForge Engine`**.
- That executable is effectively **your game/app** built on top of the engine code.
- To “use the engine”, you modify/extend the codebase and rebuild.

> Main supported platform: **Windows x64**.

---

## Features

- **Cell-based falling-sand simulation** (materials per cell).
- **Chunk/dirty-region system** to avoid updating the full grid every tick.
- **Fixed timestep simulation** with pause + single-step.
- **Deterministic RNG helpers** (coordinate + parity based) for reproducible behavior.
- **GPU rendering** (palette + GLSL shaders).
- **Brush painting tool** (radius control, axis-locked strokes).
- **Level save/load** (`.lvl`) + quick save/load bindings.
- **UI layer** (can consume mouse input).
- **Audio** via **miniaudio**.
- **File logging** to `logs/engine.log` (folder auto-created).

---

## Requirements

### Minimum
- **OS:** Windows 10 (64-bit)
- **CPU:** x64 dual-core
- **GPU:** OpenGL **3.3 Core** compatible
- **RAM:** **256 MB**
- **Disk:** ~200 MB free (project + build artifacts)
- **Display:** 1280×720

### Recommended
- **OS:** Windows 10/11 (64-bit)
- **CPU:** x64 quad-core
- **GPU:** newer drivers / OpenGL 4.x capable
- **RAM:** **512 MB**
- **Display:** 1920×1080

> Tip: On laptops with iGPU + dGPU, forcing the dedicated GPU can help avoid OpenGL driver issues.

---

## Getting Started (recommended workflow)

### 1) Fork / Clone
```bash
git clone https://github.com/msabate00/SandForge-Engine.git
```

### 2) Open with Visual Studio (CMake)
1. Open **Visual Studio 2022** (or newer)
2. `File → Open → Folder...`
3. Select: `SandForge-Engine/SandForge-Engine/`
4. Visual Studio will configure the CMake project automatically.

### 3) Build & Run
- Choose configuration: **Debug** or **Release**
- Build the default target: **`SandForge Engine`**
- Run (Local Windows Debugger)

#### Working directory / assets
The executable expects these folders to be reachable from the working directory:
- `assets/` (shaders, sprites, audio)
- `levels/` (level files)

If you run from Visual Studio, set the working directory to the folder that contains `assets/` and `levels/`
(i.e. `SandForge-Engine/SandForge-Engine/`) or ensure those folders are copied next to the built executable.

Logs are written to:
- `logs/engine.log`

---

## Alternative: Build from command line (CMake)

```bash
cd SandForge-Engine/SandForge-Engine
cmake -S . -B build
cmake --build build --config Release
```

---

## Where to put your game code

The project is structured as an engine + a “game layer”:
- Core engine systems live under `src/core/`, `src/render/`, `src/ui/`, `src/audio/`, `src/app/`
- The gameplay layer lives under `src/game/` (scene system, menus, sandbox/levels, etc.)

Typical workflow:
1. Add/modify scenes in `src/game/`
2. Use the engine API (world, materials, renderer, UI, audio)
3. Rebuild — the resulting `SandForge Engine.exe` is your game.

---

## Default Controls

### Materials
- **1**: Sand
- **2**: Water
- **3**: Stone
- **4**: Wood
- **5**: Fire
- **6**: Smoke
- **9**: Erase (Empty)

### Painting / Tools
- **LMB (hold)**: paint current material
- **Mouse Wheel**: change brush radius (1–64)
- **Shift (hold)**: lock stroke to X/Y axis (straight line)

### Simulation / Debug
- **P**: pause / resume
- **N**: step one simulation tick (when paused)
- **F1**: toggle chunk debug overlay
- **F2**: toggle hitbox debug overlay
- **F6**: clear world
- **F5**: quick save → `levels/custom/quick.lvl`
- **F9**: quick load ← `levels/custom/quick.lvl`

### Camera
- **W A S D**: pan camera
- **Ctrl + Mouse Wheel**: zoom (pixels-per-cell)

---

## Project Layout

```text
SandForge-Engine/
├─ SandForge-Engine/              # Main CMake project (open this folder in Visual Studio)
│  ├─ assets/                     # Shaders, sprites, audio
│  ├─ levels/                     # Level files (.lvl)
│  ├─ logs/                       # Runtime logs (auto-created)
│  ├─ src/
│  │  ├─ app/                     # App bootstrap, logging, utilities
│  │  ├─ core/                    # Simulation, materials, world, tools
│  │  ├─ render/                  # OpenGL renderer
│  │  ├─ ui/                      # UI layer
│  │  ├─ audio/                   # miniaudio wrapper
│  │  └─ game/                    # Game layer (scenes, menus, levels)
│  └─ third_party/                # Vendored dependencies (GLAD, miniaudio, stb...)
├─ LICENSE
└─ README.md
```

---

## License

MIT. See `LICENSE`.

---

## Third‑party

This project includes and/or fetches third-party libraries (see their own licenses), commonly:
- **GLFW** (fetched via CMake FetchContent)
- **GLAD** (vendored)
- **miniaudio** (vendored)
- **stb** headers (vendored, if used)
