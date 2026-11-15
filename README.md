# Aftermath

First-person forensic shooting reconstruction. The player reads bullet defects, ricochets, glass fractures, cartridge
cases and firearms left at a scene, runs trajectory strings, places the shooters and files a report that is scored
against what really happened.

**Engine:** Unreal Engine **5.6.1** (C++), Windows, DirectX 11 by default (DirectX 12 optional).
**Target PC:** 4-core Intel Core i5, 16 GB RAM, 4 GB graphics card (GTX 1650 / GTX 1050 Ti 4 GB / RX 570 4 GB class), 1080p.

## Quick start

1. Set up the PC once: [Docs/SETUP.md](Docs/SETUP.md) (Visual Studio 2022 with MSVC 14.38, UE 5.6.1, Git + Git LFS).
2. Clone: `git clone https://github.com/andr0n0ss7/aftermath.git C:\Dev\Aftermath`
3. Right-click `Aftermath.uproject` → **Generate Visual Studio project files** → open `Aftermath.sln` →
   **Development Editor / Win64** → Build.
4. Open `Aftermath.uproject`. Tools → Execute Python Script →
   `Plugins/AftermathCore/Content/Python/aftermath_setup.py`. Wait for `[Aftermath] Done. Press Play.`
5. Build → **Build Lighting Only**. Press **Play**.
6. Commit what the script created: `git add -A`, `git commit -m "Generated Kitchen level"`, `git push`.

## Repository layout

| Path | What it is |
| --- | --- |
| `Aftermath.uproject`, `Source/` | The Unreal project and a thin game module |
| `Plugins/AftermathCore/` | All game logic in C++: case data, ballistics, evidence, tools, firearms, surfaces, scoring, player, HUD |
| `Plugins/AftermathCore/Content/Python/` | Editor script that builds materials, the level, lights and data assets |
| `AftermathData/Data/` | Case files (ground truth), level blockouts, the surface library (23 crime-scene materials) |
| `AftermathData/Textures/` | Defect decals and glass crack masks |
| `Config/` | Engine, scalability and input settings tuned for low-end PCs |
| `Content/` | Unreal assets (created by the setup script, then edited in the editor) |
| `Docs/` | Setup, performance budgets, level design rules, firearm model pipeline |

## Controls (greybox HUD)

| Key | Action |
| --- | --- |
| WASD, mouse, Shift | Move, look, move faster |
| C / F | Crouch / flashlight |
| E or left click | Inspect the trace under the crosshair |
| 1 | Mark and photograph |
| 2 3 4 | Calipers, lead swab, copper swab |
| 5 6 7 | Seat a rod, recover the projectile, run a string |
| 8 | Call a defect: entry / exit / ricochet / not a bullet (glass: which side the shot came from) |
| 9 | Glass: examine the edge. Firearm: record its condition as found, hands off |
| 0 | Firearm: unload and make safe (do 9 first) |
| X | Leave a marked item out of the report |
| G | Place a shooter figure |
| Tab, Enter | Report, submit |

Claims go in the console (`~`) until the case board UI exists: `AMShots 5`, `AMWeapon 9mm`, `AMWeapon 38`, `AMFirst R`,
`AMStatement Contradicted`, `AMFigure 1 R standing`, `AMAngles <v> <h>`, `AMAssisted 0`, `AMTruth 1`.

## Docs

- [Docs/SETUP.md](Docs/SETUP.md): workstation setup and the daily Git routine
- [Docs/PERFORMANCE_BUDGETS.md](Docs/PERFORMANCE_BUDGETS.md): what the low-end target allows
- [Docs/LEVEL_DESIGN.md](Docs/LEVEL_DESIGN.md): how big and how detailed a scene may be, and the surface material list
- [Docs/FIREARMS_PIPELINE.md](Docs/FIREARMS_PIPELINE.md): bringing real-world firearm models in
