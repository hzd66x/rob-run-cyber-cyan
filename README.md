# ROB RUN — CYBER CYAN (UI configuration project)

Standalone Windows desktop configuration UI built with C++20, Dear ImGui and DirectX 11. It provides a configurable Visuals panel, Aim settings panel, Misc preferences, Cyber Cyan theme, color editors, sliders, selectable target-point/activation-key presets, and JSON persistence.

**Scope:** this project is a settings UI only. It does not attach to ROB RUN, inspect or modify its process/memory, inject code, read game entities, draw over the game, or automate input. The Visuals/Aim controls store preferences and demonstrate the UI; they are not connected to game telemetry or controls.

## Requirements
- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- Internet connection on first configure (CMake fetches Dear ImGui and nlohmann/json)

## Build
Open a Developer PowerShell in this folder:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Run `build\Release\RobRunCyberCyan.exe`.

## Configuration
Settings are saved to `config/rob_run_cyber_cyan.json` beside the executable's working directory. Use **Save configuration** / **Reload configuration** in Settings. The app also saves on exit when `Save on exit` is enabled.

## Notes
- DirectX 11 is used for the desktop UI renderer, not as a claim about ROB RUN's rendering API.
- The game engine, process architecture and offsets have not been validated from the Steam store URL.
