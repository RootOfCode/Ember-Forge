# Ember Forge

An alchemical smelting idle/incremental game written in **Common Lisp** using SDL2.

You start with a crumbling furnace and a pile of raw stone. Mine ore, discover alloys, unlock ancient recipes, and build a sprawling forge-town that runs itself. The core loop is:

**mine → smelt → combine → automate → prestige**

---

## Features

- **7 resource tiers** — from humble Stone up to rare Ember Crystals, plus intermediate crafted goods (Steel Plate, Rivets, Machine Parts)
- **Smelting queue** — up to 5 (expandable via upgrades) concurrent smelt jobs with real-time progress bars
- **Building system** — purchase and stack buildings that auto-produce resources every tick
- **Upgrade tree** — unlock new recipes, boost production multipliers, expand storage, and expand your operation
- **Shop** — buy persistent per-run bonuses using Coins (discount stacking, sell boosts, storage expansion, and more)
- **Prestige / Ascension** — reset for *Ember Essence* and buy permanent *Eternal Upgrades* that carry over every run
- **Random events** — cave-ins, wandering merchants, and ember pulses keep runs fresh
- **Offline progress** — up to 8 hours of production is applied when you reload your save
- **Achievements** — cosmetic milestones tracked across your session
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
| SDL2 runtime DLLs | `SDL2.dll`, `SDL2_image.dll`, `SDL2_ttf.dll` + dependencies |
| `libffi-8.dll` | Required by CFFI |

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

---

## Gameplay Overview

### Resources & Tiers

| Tier | Resources | Unlocked by |
|------|-----------|-------------|
| 0 | Stone, Coal, Coins | Start |
| 1 | Iron Ore, Copper Ore, Tin Ore | 50+ stone |
| 2 | Iron Bar, Copper Bar | Own any mine |
| 3 | Bronze Bar, Gear, Bellows, Rivets | 10 iron bars / Metalworking upgrade |
| 4 | Steel Bar, Steel Plate | Steel Alloy upgrade / Metalworking upgrade |
| 5 | Machine Part, Mythril Ore | Industrial Tools upgrade / Deep Drilling upgrade |
| 6 | Ember Crystal | 1+ Mythril Drill |
| ∞ | *(prestige loop)* | First Ember Crystal |

### Buildings

Buildings are purchased with resources and produce goods automatically every game tick. Costs scale by ×1.15 per building owned. Notable buildings:

- **Rusty Pickaxe** — mines Stone passively (0.5/s)
- **Coal Pit** — produces Coal (0.3/s)
- **Iron Mine** — extracts Iron Ore (0.2/s); unlocks at 50 Stone ever mined
- **Copper Mine** — extracts Copper Ore (0.2/s); unlocks at 50 Stone ever mined
- **Tinsmith** — refines Tin Ore for alloying (0.15/s)
- **Market Stall** — generates Coins passively (0.35/s); unlocks at 100 Stone ever mined
- **Auto-Furnace** — automatically runs iron-bar smelt jobs; adds one smelting slot
- **Bellows Workshop** — speeds up all active smelting by 10% per workshop owned
- **Mythril Drill** — unlocked by the *Deep-Core Drilling* upgrade; mines Mythril (0.05/s)

### Smelting

Open the **Forge** panel and queue up to 5 recipes simultaneously. Each job consumes input resources on enqueue and deposits outputs when complete. Slot count is expandable via the Second and Third Crucible upgrades.

| Recipe | Inputs | Output | Time | Unlock |
|--------|--------|--------|------|--------|
| Smelt Iron Bar | 3 Iron Ore + 1 Coal | 1 Iron Bar | 4 s | Always |
| Smelt Copper Bar | 3 Copper Ore + 1 Coal | 1 Copper Bar | 4 s | Always |
| Cast Gear | 1 Iron Bar | 1 Gear | 3 s | Always |
| Alloy Bronze | 2 Copper Bar + 1 Tin Ore | 1 Bronze Bar | 8 s | Own any Copper Bar + Tin Ore |
| Craft Bellows | 2 Bronze Bar + 2 Coal | 1 Bellows | 10 s | Always |
| Forge Rivets | 1 Iron Bar + 1 Coal | 8 Rivets | 6 s | Metalworking upgrade |
| Forge Steel | 2 Iron Bar + 3 Coal | 1 Steel Bar | 12 s | Steel Alloy Research upgrade |
| Machine Gears | 1 Steel Bar | 3 Gears | 6 s | 1+ Steel Bar ever |
| Hammer Steel Plate | 2 Steel Bar + 1 Coal | 1 Steel Plate | 14 s | Metalworking upgrade |
| Mint Trade Tokens | 1 Bronze Bar + 1 Gear | 250 Coins | 10 s | Coin Minting upgrade |
| Assemble Machine Part | 2 Gear + 1 Steel Plate + 10 Rivets | 1 Machine Part | 22 s | Industrial Tools upgrade |
| Condense Ember Crystal | 10 Mythril Ore + 5 Steel Bar | 1 Ember Crystal | 30 s | Own 1+ Mythril Drill |

### Upgrade Tree

Upgrades are purchased once with resources. A selection of notable upgrades:

| Upgrade | Effect |
|---------|--------|
| Sturdy Gloves | Double click mining power |
| Miner's Pouch | Double manual gathering yield |
| Quick Hands | Manual actions 0.5 s faster |
| Hardened Pickaxes | Pickaxes mine 2× faster |
| Gear-Driven Picks | Pickaxes mine 75% faster |
| Deep Coal Seams | Coal Pits produce 50% more coal |
| Furnace Tuning | All smelting 25% faster |
| Second Crucible | +1 smelting slot |
| Third Crucible | +1 smelting slot |
| Market Network | Market Stalls generate 2× coins |
| Bright Signage | Market Stalls generate +50% coins |
| Reinforced Sacks | Stone/Coal storage +50% |
| Ore Crates | Ore storage +50% |
| Precision Molds | Gears, Bellows, and Rivets craft 2× output |
| Steel Alloy Research | Unlocks steel smelting recipe |
| Metalworking | Unlocks Steel Plates and Rivets recipes |
| Coin Minting | Unlocks the Trade Token minting recipe |
| Ancient Veins | +50% ore production from all mines |
| Deep-Core Drilling | Unlocks Mythril Drill building |
| Industrial Tools | Unlocks Machine Parts; Mythril Drills +50% |

### Shop

The **Shop** panel lets you spend Coins on stackable per-run bonuses. Effects apply immediately and persist until you Ascend.

| Item | Max Level | Effect per level |
|------|-----------|-----------------|
| Haggling Lessons | 10 | Shop prices −5% (floor: 60% of base) |
| Salesmanship | 12 | Sell values +10% (cap: 3×) |
| Warehouse Lease | 8 | All storage caps +15% |
| Smelter's Coupon | 10 | Smelting speed +10% |
| Foreman Contract | 10 | All building production +7% |

Shop costs scale by a per-item factor each level (1.6×–2.0×). Buying resources directly from the shop is also available for any resource that has a sell value — prices are calibrated to prevent buy-then-sell arbitrage.

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

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `1` | Forge panel |
| `2` | Mine panel |
| `3` | Upgrades panel |
| `4` | Recipes panel |
| `5` | Shop panel |
| `6` | Prestige panel |
| `Esc` | Main menu |
| `F11` | Toggle fullscreen |

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
│   ├── shop.lisp                # Shop items, buy/sell logic, economy modifiers
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
├── scripts/
│   └── build-release.lisp       # Release build script
├── build.sh                     # Linux build script
├── build.bat                    # Windows build script
├── run.sh                       # Run from source (Linux)
└── logo.png
```

---

## Technical Notes

- **Language**: Common Lisp (SBCL recommended)
- **Rendering**: SDL2 primitives + TTF fonts — no external sprite sheets besides some basic background images
- **Window size**: 1280 × 768
- **Target frame rate**: 60 fps (16 ms tick)
- **All production math uses `double-float`** to avoid integer overflow at high prestige counts
- **Delta-time is capped at 0.5 s** to prevent large jumps after alt-tab
- **Autosave** every 60 seconds of real time to `~/.local/share/ember-forge/save.lisp`
- **Multiple save slots** supported (`save-slot` field in game state)
- **Offline production** computed on load (capped at 8 hours)

---

## License

See individual font subdirectories under `assets/fonts/` for third-party font licenses.
