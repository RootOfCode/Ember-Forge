# Ember Forge

An alchemical smelting idle/incremental game written in **Common Lisp** using SDL2.

You start with a crumbling furnace and a pile of raw stone. Mine ore, discover alloys, unlock ancient recipes, and build a sprawling forge-town that runs itself. The core loop is:

**mine → smelt → combine → automate → prestige**

---

## Features

- **6 resource tiers** — from humble Stone up to rare Ember Crystals
- **Smelting queue** — up to 5 (expandable) concurrent smelt jobs with real-time progress bars
- **Building system** — purchase and stack buildings that auto-produce resources every tick
- **Upgrade tree** — unlock new recipes, boost production multipliers, and expand your operation
- **Prestige / Ascension** — reset for *Ember Essence* and buy permanent *Eternal Upgrades* that carry over every run
- **Random events** — cave-ins, wandering merchants, and ember pulses keep runs fresh
- **Offline progress** — up to 8 hours of production is applied when you reload your save
- **Human-readable save file** — stored as a plain Lisp plist at `~/.local/share/ember-forge/save.lisp`

---

## Requirements

### Linux

| Dependency | Notes |
|---|---|
| SBCL | Common Lisp compiler |
| SDL2 | Windowing & rendering |
| SDL2_ttf | Font rendering |
| SDL2_image | Image loading |
| Quicklisp | With `sdl2`, `sdl2-ttf`, `sdl2-image`, `alexandria` |

### Windows

| Dependency | Notes |
|---|---|
| SBCL for Windows | [sbcl.org](https://www.sbcl.org/) |
| Quicklisp | [quicklisp.org](https://www.quicklisp.org/) |
| SDL2 runtime DLLs | `SDL2.dll`, `SDL2_image.dll`, `SDL2_ttf.dll` + dependencies (bundled in `dlls/`) |
| `libffi-8.dll` | Required by CFFI — bundled in `dlls/libffi/` |

All required Windows DLLs are already bundled in the `dlls/` directory of this repository.

---

## Running & Building

### Run from source (Linux / macOS)

```bash
./run.sh
```

### Build a Linux binary

```bash
./build.sh
./dist/ember-forge
```

### Build a Windows binary (on Windows)

```bat
build.bat
dist\ember-forge.exe
```

### Build a Windows binary (on Linux via Wine)

Requires Wine with a Windows SBCL installation inside it, plus a MinGW-w64 cross-compiler (`x86_64-w64-mingw32-gcc`) to handle Quicklisp/CFFI grovel steps.

See `scripts/wine/README.md` for full setup instructions.

```bash
./scripts/wine/gcc-wrap.sh   # configure the Wine GCC wrapper once
./build.sh --target windows
wine dist/ember-forge.exe
```

---

## Gameplay Overview

### Resources & Tiers

| Tier | Resources | Unlocked by |
|------|-----------|-------------|
| 0 | Stone, Coal, Coins | Start |
| 1 | Iron Ore, Copper Ore, Tin | 50+ stone |
| 2 | Iron Bar, Copper Bar | Own any mine |
| 3 | Bronze, Gear, Bellows | 10 iron bars |
| 4 | Steel | Steel Alloy upgrade |
| 5 | Mythril | Deep Drilling upgrade |
| 6 | Ember Crystal | 50+ Mythril ever produced |
| ∞ | *(prestige loop)* | First Ember Crystal |

### Buildings

Buildings are purchased with resources and produce goods automatically every game tick. Costs scale by ×1.15 per building owned. Notable buildings:

- **Rusty Pickaxe** — mines Stone passively
- **Coal Pit / Iron Mine / Copper Mine** — dig up raw ore
- **Auto-Furnace** — automatically runs iron-bar smelt jobs
- **Bellows Workshop** — speeds up all active smelting by 10% per workshop
- **Mythril Drill** — unlocked by the *Deep-Core Drilling* upgrade

### Smelting

Open the **Forge** panel and queue up to 5 recipes simultaneously. Each job consumes input resources on enqueue and deposits outputs when complete. Slot count is expandable via upgrades.

| Recipe | Inputs | Output | Time |
|--------|--------|--------|------|
| Smelt Iron Bar | 3 Iron Ore + 1 Coal | 1 Iron Bar | 4 s |
| Smelt Copper Bar | 3 Copper Ore + 1 Coal | 1 Copper Bar | 4 s |
| Alloy Bronze | 2 Copper Bar + 1 Tin | 1 Bronze | 8 s |
| Forge Steel | 2 Iron Bar + 3 Coal | 1 Steel | 12 s |
| Cast Gear | 1 Iron Bar | 1 Gear | 3 s |

### Prestige (Ascension)

Once you produce your first **Ember Crystal**, the **Ascend** button lights up. Ascending resets resources, buildings, and upgrades in exchange for **Ember Essence**:

```
Essence earned = floor(sqrt(total_ember_ever_produced)) + (ascensions × 2)
```

Essence is spent in the Prestige panel on **Eternal Upgrades** that persist across all future runs:

| Upgrade | Cost | Effect |
|---------|------|--------|
| Eternal Miner | 5 | Start each run with 1 free Pickaxe |
| Ember Memory | 10 | Keep 10% of coins on reset |
| Forge Mastery | 25 | All smelting permanently 25% faster |
| Ancient Veins | 50 | +50% all ore production permanently |
| Twin Furnaces | 100 | Start with Auto-Furnace already built |

### Random Events

Events fire roughly every 5–15 minutes and present a modal choice:

- **Cave-In** — spend stone to repair or lose iron production temporarily
- **Wandering Merchant** — trade refined bars for rarer materials
- **Ember Pulse** — double smelt speed for 30 seconds

---

## Project Structure

```
ember-forge/
├── ember-forge.asd              # ASDF system definition
├── src/
│   ├── main.lisp                # Entry point, game loop (60 fps)
│   ├── state.lisp               # Global game-state struct
│   ├── resources.lisp           # Resource definitions & registry
│   ├── recipes.lisp             # Recipe definitions & smelt queue logic
│   ├── buildings.lisp           # Building definitions & tick logic
│   ├── upgrades.lisp            # Upgrade tree definitions
│   ├── events.lisp              # Random event system
│   ├── prestige.lisp            # Prestige / ascension logic
│   ├── ui/
│   │   ├── renderer.lisp        # Master render dispatcher
│   │   ├── panel-forge.lisp     # Forge panel (click-to-mine + smelt queue)
│   │   ├── panel-mine.lisp      # Mine / resource overview panel
│   │   ├── panel-upgrades.lisp  # Upgrade tree panel
│   │   ├── panel-recipes.lisp   # Recipe browser panel
│   │   ├── panel-shop.lisp      # Building shop panel
│   │   ├── panel-prestige.lisp  # Prestige & eternal upgrades panel
│   │   ├── menu.lisp            # Main menu
│   │   ├── widgets.lisp         # Button, label, progress-bar primitives
│   │   ├── tooltip.lisp         # Hover tooltip system
│   │   ├── notifs.lisp          # Floating toast notifications
│   │   ├── backgrounds.lisp     # Background image rendering
│   │   └── theme.lisp           # Color palette & font handles
│   └── util/
│       ├── math.lisp            # lerp, clamp, large-number formatting
│       ├── timer.lisp           # Delta-time tracking
│       ├── fonts.lisp           # Font loading helpers
│       ├── options.lisp         # User options / settings
│       └── save.lisp            # Serialise / deserialise state to file
├── assets/
│   ├── backgrounds/             # Panel background images (PNG)
│   └── fonts/                   # Bundled TTF/OTF fonts
├── dlls/                        # Bundled Windows runtime DLLs
│   ├── sdl2/
│   ├── sdl2_image/
│   ├── sdl2_ttf/
│   ├── libffi/
│   ├── libfreetype-6/
│   ├── libstdc++-6/
│   ├── libgcc_s_seh-1/
│   ├── libwinpthread-1/
│   └── zlib1/
├── scripts/
│   ├── wine/                    # Cross-compilation helpers (Linux → Windows)
│   └── build-release.lisp       # Release build script
├── build.sh                     # Linux build script
├── build.bat                    # Windows build script
├── run.sh                       # Run from source (Linux)
└── logo.png
```

---

## Technical Notes

- **Language**: Common Lisp (SBCL recommended)
- **Rendering**: SDL2 primitives + TTF fonts — no external sprite sheets
- **Window size**: 1280 × 768
- **Target frame rate**: 60 fps (16 ms tick)
- **All production math uses `double-float`** to avoid integer overflow at high prestige counts
- **Delta-time is capped at 0.5 s** to prevent large jumps after alt-tab
- **Autosave** every 60 seconds of real time to `~/.local/share/ember-forge/save.lisp`
- **Offline production** computed on load (capped at 8 hours)

---

## License

See individual DLL subdirectories under `dlls/` for third-party library licenses.
