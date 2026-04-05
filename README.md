# Ember Forge

A cross-platform idle/incremental crafting game built in C with SDL2. Mine stone, smelt iron, forge steel, discover mythril, and ascend through multiple runs to unlock permanent upgrades that reshape each new playthrough.

---

## Table of Contents

- [Building](#building)
  - [Dependencies](#dependencies)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows (cross-compile)](#windows-cross-compile)
  - [Windows (native MinGW)](#windows-native-mingw)
  - [Makefile options](#makefile-options)
- [Project Layout](#project-layout)
- [Gameplay](#gameplay)
  - [Panels](#panels)
  - [Resources](#resources)
  - [Buildings](#buildings)
  - [Smelting Queue](#smelting-queue)
  - [Dedicated Crucibles](#dedicated-crucibles)
  - [Recipes](#recipes)
  - [Upgrades](#upgrades)
  - [Shop](#shop)
  - [Events](#events)
  - [Achievements](#achievements)
  - [Prestige (Ascension)](#prestige-ascension)
- [Gather Timing](#gather-timing)
- [Save System](#save-system)
- [Adding Game Content](#adding-game-content)
  - [New Resource](#new-resource)
  - [New Building](#new-building)
  - [New Upgrade](#new-upgrade)
  - [New Recipe](#new-recipe)
  - [New Shop Item](#new-shop-item)
- [Platform Notes](#platform-notes)

---

## Building

### Dependencies

| Library | Minimum version | Notes |
|---|---|---|
| SDL2 | 2.0.14 | Core window / renderer |
| SDL2_image | 2.0.5 | PNG backgrounds |
| SDL2_ttf | 2.0.15 | Font rendering |

A C11-capable compiler is required (`gcc >= 7`, `clang >= 6`, `MinGW-w64 >= 8`).

---

### Linux

```bash
# Debian / Ubuntu
sudo apt install gcc libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

# Fedora / RHEL
sudo dnf install gcc SDL2-devel SDL2_image-devel SDL2_ttf-devel

# Arch
sudo pacman -S gcc sdl2 sdl2_image sdl2_ttf

make          # or: make linux
```

The binary is written to `bin/ember_forge`.

---

### macOS

```bash
brew install sdl2 sdl2_image sdl2_ttf
make macos
```

The binary is written to `bin/ember_forge`.

> **Apple Silicon note:** Homebrew installs to `/opt/homebrew` on M-series Macs. The Makefile calls `brew --prefix` automatically so no extra flags are needed.

---

### Windows (cross-compile)

Cross-compile an `.exe` from a Linux host using MinGW-w64:

```bash
sudo apt install gcc-mingw-w64-x86-64
sudo apt install libsdl2-dev:mingw-w64 libsdl2-image-dev:mingw-w64 libsdl2-ttf-dev:mingw-w64

make windows
```

The binary is written to `bin/ember_forge.exe`. If your SDL2 MinGW headers and libs live somewhere non-standard:

```bash
make windows SDL2_MINGW_PREFIX=/path/to/SDL2-2.30.0-win32-x86_64
```

---

### Windows (native MinGW)

Install [MSYS2](https://www.msys2.org/), then inside the MINGW64 shell:

```bash
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-SDL2 \
          mingw-w64-x86_64-SDL2_image \
          mingw-w64-x86_64-SDL2_ttf \
          make

make linux CC=gcc       # 'linux' target works fine under MSYS2/MINGW64
```

---

### Makefile options

| Variable | Default | Purpose |
|---|---|---|
| `CC` | `gcc` / `clang` / `x86_64-w64-mingw32-gcc` | Compiler |
| `OPT` | `-O2` | Optimisation level |
| `V` | `0` | Set to `1` for verbose compiler output |
| `SDL2_MINGW_PREFIX` | auto-detected | SDL2 root for Windows cross-compile |

```bash
make OPT=-O0 V=1        # debug build, verbose
make CC=clang linux     # force clang
make clean              # remove obj/ and bin/
```

---

## Project Layout

```
Ember Forge/
├── Makefile
├── README.md
├── include/
│   ├── ef_content.h    <- EF_EACH_RESOURCE table: all resource definitions in one place
│   ├── ef_defs.h       <- all enums: ResourceId, BuildingId, RecipeId, UpgradeId, ...
│   ├── ef_state.h      <- GameState struct, SmeltJob, CrucibleSlot, ...
│   ├── ef_platform.h   <- platform abstraction (mkdir, dirent, strcasecmp, ...)
│   ├── ef_buildings.h
│   ├── ef_recipes.h
│   ├── ef_resources.h
│   ├── ef_upgrades.h
│   ├── ef_save.h
│   └── ...
├── src/
│   ├── main.c          <- SDL2 event loop, render dispatch
│   ├── ef_game.c       <- tick_game(), manual mine/gather, offline progress
│   ├── ef_buildings.c  <- building definitions, production tick
│   ├── ef_recipes.c    <- recipe definitions, smelt queue, crucible tick
│   ├── ef_upgrades.c   <- upgrade definitions and effects
│   ├── ef_shop.c       <- shop item definitions and purchase logic
│   ├── ef_prestige.c   <- ascension, essence, eternal upgrades
│   ├── ef_events.c     <- random events, achievement checks
│   ├── ef_resources.c  <- resource caps, production, sell values
│   ├── ef_save.c       <- save / load (key=value .sav format)
│   ├── ef_ui.c         <- all panel rendering
│   └── ...
├── obj/                <- compiled .o files (generated)
├── bin/                <- output binary (generated)
└── assets/
    ├── backgrounds/    <- per-panel PNG backgrounds
    └── fonts/
```

---

## Gameplay

The game window is split into a main panel on the left and a resource sidebar on the right. Six panels are available, switched via tabs at the top of the screen.

### Panels

| Panel | Purpose |
|---|---|
| **Forge** | Click-mining, smelting queue, dedicated crucibles, buildings |
| **Mine** | Manual gathering of ores and coal |
| **Upgrades** | One-time research upgrades |
| **Recipes** | Browse and queue smelting recipes |
| **Shop** | Repeatable coin-purchased perks |
| **Prestige** | Ascend to earn Essence and spend it on eternal upgrades |

---

### Resources

Resources are displayed in the right-hand sidebar and unlock progressively as you produce them for the first time. Each has a storage cap that can be raised by upgrades, shop items, and prestige bonuses.

| Resource | Tier | Notes |
|---|---|---|
| Stone | 0 | First resource; mined by clicking and by Pickaxes |
| Coal | 0 | Fuel for smelting; gathered or produced by Coal Pits |
| Coins | 0 | Currency for buildings, upgrades, and the shop |
| Iron Ore | 1 | Smelted into Iron Bars |
| Copper Ore | 1 | Smelted into Copper Bars |
| Tin Ore | 1 | Produced by Tinsmiths; used in Bronze alloying |
| Iron Bar | 2 | Core processed material; needed for many buildings and upgrades |
| Copper Bar | 2 | Needed for Bronze alloying and several upgrades |
| Bronze Bar | 3 | Alloy requiring Copper Bars and Tin |
| Gear | 3 | Crafted from Iron Bars; used in high-tier buildings |
| Bellows | 3 | Crafted from Bronze; installed in Bellows Workshops |
| Steel Bar | 4 | High-tier alloy unlocked by research |
| Steel Plate | 4 | Requires Steel and Coal; needed for Machine Parts |
| Rivets | 3 | Batch-produced from Iron Bars; used in late assembly |
| Machine Part | 5 | End-tier component assembled from Gears, Plates, and Rivets |
| Mythril Ore | 5 | Rare late-game ore; mined by Mythril Drills or gathered manually |
| Ember Crystal | 6 | Prestige material; condensed from Mythril and Steel |

---

### Buildings

Buildings are purchased on the Forge panel and produce resources or grant effects automatically. Each building costs more per copy owned (x1.15 scaling by default).

| Building | Produces | Base cost | Notes |
|---|---|---|---|
| **Rusty Pickaxe** | 0.5 stone/s | 10 coins | Always available |
| **Coal Pit** | 0.3 coal/s | 50 coins, 20 stone | |
| **Iron Mine** | 0.2 iron/s | 200 coins, 100 stone | Unlocks once you own a Coal Pit |
| **Copper Mine** | 0.2 copper/s | 250 coins, 120 stone | |
| **Tinsmith** | 0.15 tin/s | 400 coins, 5 copper bars | |
| **Auto-Furnace** | — | 500 coins, 200 stone, 5 gears | Auto-runs Iron Bar jobs; adds +1 smelt slot per furnace |
| **Bellows Workshop** | — | 1,000 coins, 10 bronze, 3 bellows | +10% smelting speed per workshop (15% with Bellows Efficiency upgrade) |
| **Market Stall** | 0.35 coins/s | 80 coins, 60 stone, 20 coal | |
| **Mythril Drill** | 0.05 mythril/s | 50,000 coins, 50 steel, 30 gears | Requires Deep-Core Drilling upgrade |

Production rates are base values per building, multiplied by the building's own upgrade multiplier, `prod_mult` (from essence and upgrades), and relevant research bonuses.

---

### Smelting Queue

The Smelting Queue sits on the left side of the Forge panel. All queued jobs advance simultaneously at the same speed. Inputs are spent when a job is queued; cancelling a job refunds them.

**Slot count:** 5 base + `smelt_slots_extra` (from Second/Third Crucible upgrades) + Auto-Furnace count.

**Speed multiplier** is calculated as:

```
smelt_speed_mult x ember_pulse x forge_mastery x bellows_bonus
```

- `smelt_speed_mult` grows with Smelter's Coupon (shop), Furnace Tuning (upgrade), and Prestige essence.
- Ember Pulse event doubles speed for 30 seconds while active.
- Forge Mastery eternal upgrade adds a permanent 1.25x multiplier.
- Each Bellows Workshop adds 10% (15% with Bellows Efficiency upgrade).

---

### Dedicated Crucibles

Dedicated Crucibles are persistent smelting slots each locked to a single recipe that runs and restarts automatically — no manual re-queuing needed.

**Unlocking:** Buy *Dedicated Crucible* in the Shop (requires 10 Iron Bars ever produced). Each purchase adds one slot; maximum 4 total.

**Forge panel — right column:**
- Each slot shows its pinned recipe name, a progress bar, and `<` / `>` arrows to cycle through all available recipes.
- Switching the recipe restarts the slot immediately.
- If inputs are exhausted mid-run, the slot displays *"Waiting for resources..."* and retries automatically each tick.

**Recipes panel:**
- A **Pin** button appears beside the Smelt button on each recipe card (only visible when at least one crucible slot is owned).
- Pin assigns the recipe to the first empty slot (or slot 0 if all are occupied) and starts it immediately.

Crucibles share the same speed multiplier as the regular queue and respect the Precision Molds upgrade (2x output for Gears, Bellows, and Rivets). Their full state is saved and restored between sessions.

---

### Recipes

Recipes are viewed and queued from the Recipes panel. Each card shows inputs, outputs, duration, a Smelt button, and (if crucibles are unlocked) a Pin button.

| Recipe | Inputs | Outputs | Duration | Unlock |
|---|---|---|---|---|
| Smelt Iron Bar | 3 iron, 1 coal | 1 iron bar | 4 s | Always |
| Smelt Copper Bar | 3 copper, 1 coal | 1 copper bar | 4 s | Always |
| Cast Gear | 1 iron bar | 1 gear | 3 s | Always |
| Alloy Bronze | 2 copper bars, 1 tin | 1 bronze bar | 8 s | Possess 1 copper bar and 1 tin |
| Craft Bellows | 2 bronze, 2 coal | 1 bellows | 10 s | Always |
| Forge Steel | 2 iron bars, 3 coal | 1 steel bar | 12 s | Steel Alloy Research upgrade |
| Machine Gears | 1 steel | 3 gears | 6 s | Produced >= 1 steel ever |
| Forge Rivets | 1 iron bar, 1 coal | 8 rivets | 6 s | Metalworking upgrade |
| Hammer Steel Plate | 2 steel, 1 coal | 1 steel plate | 14 s | Metalworking upgrade |
| Assemble Machine Part | 2 gears, 1 steel plate, 10 rivets | 1 machine part | 22 s | Industrial Tools upgrade |
| Mint Trade Tokens | 1 bronze, 1 gear | 250 coins | 10 s | Coin Minting upgrade |
| Condense Ember Crystal | 10 mythril, 5 steel | 1 ember crystal | 30 s | Own >= 1 Mythril Drill |

The **Precision Molds** upgrade doubles the output of Gear, Bellows, and Rivet recipes in both the queue and dedicated crucibles.

---

### Upgrades

Upgrades are one-time purchases from the Upgrades panel. They appear when their unlock condition is met and disappear once bought.

**Mining and gathering**

| Upgrade | Effect | Unlock condition |
|---|---|---|
| Sturdy Gloves | Click mining power x2 | Produced >= 20 stone |
| Miner's Pouch | Manual gather yield x2 | Produced >= 20 stone |
| Quick Hands | Manual actions 0.5 s faster | Own >= 2 Pickaxes |
| Hardened Pickaxes | Pickaxes produce x2 | Own >= 5 Pickaxes |
| Gear-Driven Picks | Pickaxes produce x1.75 more | Own >= 20 Pickaxes |
| Deep Coal Seams | Coal Pits +50% coal | Own >= 3 Coal Pits |
| Coal Dust Filters | Coal Pits +75% coal | Own >= 10 Coal Pits |
| Reinforced Sacks | Stone/Coal storage +50% | Produced >= 200 stone |
| Ore Crates | Ore storage +50% | Own >= 1 Iron Mine |
| Ancient Veins | All ore production +50% | Produced >= 5 steel |
| Ore Sorting | Iron/Copper/Tin mines +40% | Produced >= 10 bronze |

**Smelting**

| Upgrade | Effect | Unlock condition |
|---|---|---|
| Furnace Tuning | Smelting speed +25% | Produced >= 5 iron bars |
| Second Crucible | +1 smelt slot | Own >= 1 Auto-Furnace |
| Third Crucible | +1 smelt slot | Own Second Crucible |
| Precision Molds | Gears/Bellows/Rivets x2 output | Produced >= 10 gears |
| Bellows Efficiency | Bellows Workshops grant 15% speed each instead of 10% | Own >= 1 Bellows Workshop |

**Research unlocks**

| Upgrade | Effect | Unlock condition |
|---|---|---|
| Steel Alloy Research | Unlocks Steel smelting recipe | Produced >= 20 iron bars |
| Metalworking | Unlocks Steel Plate and Rivet recipes | Produced >= 5 steel |
| Coin Minting | Unlocks Mint Trade Tokens recipe | Own >= 2 Market Stalls |
| Deep-Core Drilling | Unlocks Mythril Drill building | Produced >= 10 steel |
| Industrial Tools | Unlocks Machine Part recipe; Mythril Drills +50% | Produced >= 5 steel plates |

**Economy and global production**

| Upgrade | Effect | Unlock condition |
|---|---|---|
| Market Network | Market Stalls x2 coins | Own >= 1 Market Stall |
| Bright Signage | Market Stalls +50% coins | Own >= 1 Market Stall |
| Assembly Lines | All building production +15% | Produced >= 1 machine part |
| Mechanized Drills | Mythril Drills x2 mythril | Own >= 1 Mythril Drill |

---

### Shop

The Shop sells repeatable perks for coins. Each item can be purchased multiple times and its cost scales with level.

| Item | Effect per purchase | Max level | Unlock condition |
|---|---|---|---|
| Haggling Lessons | Shop prices -5% | 10 | Produced >= 120 coins |
| Salesmanship | Sell values +10% | 12 | Produced >= 150 coins |
| Warehouse Lease | All storage caps +15% | 8 | Produced >= 200 stone |
| Smelter's Coupon | Smelting speed +10% | 10 | Produced >= 5 iron bars |
| Foreman Contract | All building production +7% | 10 | Produced >= 5 bronze |
| Dedicated Crucible | Unlock one auto-repeat crucible slot | 4 | Produced >= 10 iron bars |

The shop also lets you buy individual resources directly at a markup over their sell value, subject to your current storage cap and coin balance.

---

### Events

Random events fire roughly every 90 seconds when no event is already active. Each presents a modal overlay with a short description and two or three choices. Choosing an option dismisses the overlay and applies its effect immediately.

| Event | Description | Choices |
|---|---|---|
| **Cave-In!** | A tremor collapses a mine tunnel. | Shore it up (spend 50 stone, no penalty) / Abandon it (-iron production for 60 s) |
| **Wandering Merchant** | A hooded trader offers rare goods. | Trade 5 steel for 3 mythril / Trade 10 bronze for 500 coins / Pass |
| **Ember Pulse** | An ember crystal resonates and the forge glows hotter. | Harness the pulse (x2 smelting speed for 30 s) / Ignore it |
| **Scrap Windfall** | A broken caravan leaves salvageable scrap. | Salvage (+5 iron bars, +3 copper bars) / Sell to market (+350 coins) |
| **Ancient Cache** | A sealed coffer cracks open. | Take gears (+12) / Take steel plates (+4) / Take rivets (+60) |

---

### Achievements

Achievements are tracked silently and announced in the notification bar when earned. They are informational only.

| Achievement | Condition |
|---|---|
| First Spark | Smelt your first bar |
| Iron Will | Own 10 Iron Mines |
| The Alchemist | Discover the Bronze recipe |
| Deep Echo | Extract your first Mythril |
| Ember Awakened | Condense your first Ember Crystal |
| Eternal Flame | Ascend 5 times |
| Stone Stockpile | Produce 1,000 stone |
| Coal Baron | Produce 500 coal |
| Pickaxe Army | Own 25 Pickaxes |
| Furnace Fleet | Own 3 Auto-Furnaces |
| Bellows Business | Own 2 Bellows Workshops |
| Merchant Prince | Produce 10,000 coins |
| Gearhead | Produce 50 gears |
| Airflow Artisan | Craft 10 bellows |
| Bronze Age | Produce 25 bronze bars |
| Steelworks | Produce 25 steel bars |
| Plate Press | Hammer 10 steel plates |
| Rivet Rain | Forge 200 rivets |
| Parts Bin | Assemble 3 machine parts |
| Shopaholic | Buy 5 shop perks total |
| Queue Master | Fill 8 smelt slots simultaneously |
| Tinkerer | Buy 10 upgrades |
| Industrial Revolution | Buy the Assembly Lines upgrade |

---

### Prestige (Ascension)

Ascending resets all resources, buildings, upgrades, shop purchases, and the smelting queue, but rewards **Essence** — a permanent currency spent on eternal upgrades that persist across every future run.

**Requirement:** Produce at least 1 Ember Crystal in the current run (visible on the Prestige panel once met).

**Essence gained:** `floor(sqrt(ember_crystals_ever)) + (ascensions x 2)`. Each subsequent run awards more essence for the same Ember output, so progress accelerates with experience.

**Coin carry-over:** With the Ember Memory eternal upgrade, 10% of your coin balance is kept on ascension. All other resources reset to zero, with a 25-coin starting grant applied first.

#### Passive essence bonuses

Unspent Essence sitting in your pool provides passive scaling bonuses each run:

| Stat | Bonus per Essence |
|---|---|
| All building production (`prod_mult`) | +5% |
| Click mining power (`click_mult`) | +2% |
| Manual gather yield (`gather_mult`) | +1.5% |
| Smelting speed (`smelt_speed_mult`) | +1% |

#### Eternal Upgrades

Eternal upgrades are purchased with Essence on the Prestige panel and are never lost on ascension.

| Upgrade | Cost | Effect |
|---|---|---|
| Eternal Miner | 5 E | Start every run with 1 free Pickaxe |
| Ember Memory | 10 E | Keep 10% of coins on Ascend |
| Forge Mastery | 25 E | All smelting permanently 25% faster |
| Ancient Veins | 50 E | All ore production permanently +50% |
| Twin Furnaces | 100 E | Start every run with 1 free Auto-Furnace |

---

## Gather Timing

Manual gathering is triggered from the Mine panel. Durations are defined in the `gather_time` column of `EF_EACH_RESOURCE` in `include/ef_content.h`. A value of `0.0` means the resource cannot be manually gathered.

| Resource | Base duration |
|---|---|
| Stone (click on Forge) | 5.0 s |
| Stone (Mine panel) | 4.0 s |
| Coal | 6.0 s |
| Iron Ore | 8.0 s |
| Copper Ore | 8.0 s |
| Tin Ore | 10.0 s |
| Mythril Ore | 18.0 s |

The actual in-game duration is `base_time / gather_mult`. The `gather_mult` stat begins at 1.0 and increases with Miner's Pouch (x2), Prestige essence (+1.5% per point), and any future upgrades. Higher `gather_mult` means shorter waits.

---

## Save System

Saves are plain `key=value` text files. The game autosaves on a configurable interval (default 60 s) and writes on quit.

**Save file:** `saves/slotN/save.sav` (N = 1, 2, or 3)
**Options file:** `saves/options.sav`

Three independent save slots are supported; the active slot is chosen from the main menu. Slot names can be customised in the options screen.

**Offline progress:** On reload, the game calculates time elapsed since the last save (capped at 8 hours) and applies passive building production for that period. Smelting jobs and crucibles do not advance offline.

### Format reference

```
version=1
saved_at=1743900000
tick=12345
resource.stone=450.0
cap.stone=750.0
ever.stone=1200.0
unlocked=stone coal coins iron copper
building.pickaxe=5 1
bld_mult.pickaxe=2.0
smelt_queue_len=2
smelt_queue.0=iron_bar 0.42 4.0 manual
smelt_queue.1=gear 0.10 3.0 auto
crucibles_purchased=2
crucible.0=copper_bar 0.67 0
crucible.1=gear 0.00 1
upgrades=sturdy_gloves quick_hands hardened_pickaxes furnace_tuning
eternal_upgrades=eternal_miner ember_memory
achievements=first_spark stone_stockpile iron_will
shop_lvl.haggling=3
shop_lvl.smelters_coupon=2
shop_lvl.dedicated_crucible=2
sell_mult=1.21
shop_discount=0.857
essence=18.0
ascensions=2
prod_mult=1.9
smelt_speed_mult=1.25
active_panel=recipes
```

`smelt_queue.N` format: `recipe_key progress duration source` (source is `manual` or `auto`).
`crucible.N` format: `recipe_key progress stalled` (stalled is `1` if waiting for inputs, `0` otherwise).

Legacy `.lisp` saves from the original Lisp version are read as a fallback on first load. After the first write from this C version, only the `.sav` format is used going forward.

---

## Adding Game Content

All game entities use small DSL macros so adding content only touches a few files.

### New Resource

1. Add an enum entry to `ResourceId` in `include/ef_defs.h` (before `RES_COUNT`).
2. Add one row to `EF_EACH_RESOURCE` in `include/ef_content.h`:

```c
X(RES_SILVER, "silver", "Silver Ore", PAL_SILVER, 2, 150.0, 3.5, 8.0)
//  ^id         ^key     ^display      ^color       ^tier ^cap  ^sell ^gather_time
```

Set `gather_time` to `0.0` for resources that cannot be manually gathered (bars, processed goods).

3. Add a matching string to `resource_keys[]` in `src/ef_defs.c`.

The save system, resource sidebar, milestone tracking, and unlock logic all derive from `EF_EACH_RESOURCE` automatically.

---

### New Building

1. Add an entry to `BuildingId` in `include/ef_defs.h` (before `BLD_COUNT`).
2. Add a matching string to `building_keys[]` in `src/ef_defs.c`.
3. Add a `BUILDING()` or `BUILDING_NP()` call inside `buildings_init()` in `src/ef_buildings.c`:

```c
// With automatic per-second production:
BUILDING(BLD_SILVER_PAN, "Silver Pan",
    "Slowly pans silver from river sediment.",
    1.15, unlock_silver_pan,
    COSTS({RES_COINS, 800.0}, {RES_STONE, 50.0}),
    PRODS({RES_SILVER, 0.08}));

// Effect handled in game logic, no auto-production:
BUILDING_NP(BLD_STEAM_HAMMER, "Steam Hammer",
    "Speeds up smelting by 20% per hammer.",
    1.20, NULL,
    COSTS({RES_COINS, 5000.0}, {RES_STEEL, 20.0}, {RES_MACHINE_PART, 2.0}));
```

The cost scale factor (1.15 above) is the per-copy price multiplier applied on each purchase.

---

### New Upgrade

1. Add an entry to `UpgradeId` in `include/ef_defs.h` (before `UPG_COUNT`).
2. Add a matching string to `upgrade_keys[]` in `src/ef_defs.c`.
3. Add an `UPGRADE()` call inside `upgrades_init()` in `src/ef_upgrades.c`:

```c
UPGRADE(UPG_SILVER_RUSH, "Silver Rush",
    "Silver Pans produce 2x silver.",
    ul_silver_1,   /* unlock predicate -- NULL means always visible */
    fx_silver_x2,  /* effect function  -- NULL means passive/unlock-only */
    COSTS({RES_COINS, 1500.0}, {RES_SILVER, 30.0}));
```

---

### New Recipe

1. Add an entry to `RecipeId` in `include/ef_defs.h` (before `RCP_COUNT`).
2. Add a matching string to `recipe_keys[]` in `src/ef_defs.c`.
3. Add a `RECIPE()` call inside `recipes_init()` in `src/ef_recipes.c`:

```c
RECIPE(RCP_SILVER_BAR, "Smelt Silver Bar",
    NULL,   /* unlock predicate -- NULL means always available */
    5.0,    /* duration in seconds */
    INPUTS({RES_SILVER, 3.0}, {RES_COAL, 1.0}),
    OUTPUTS({RES_SILVER_BAR, 1.0}));
```

New recipes appear automatically in the Recipes panel, are queueable in the smelt queue, and can be pinned to dedicated crucible slots with no extra steps.

---

### New Shop Item

1. Add an entry to `ShopItemId` in `include/ef_defs.h` and increment `SHOP_COUNT`.
2. Add a matching string to `shop_item_keys[]` in `src/ef_defs.c`.
3. Write an unlock predicate and an apply function, then add an entry to the `shop_items[]` table in `src/ef_shop.c`:

```c
static void apply_my_item(GameState *state, int new_level) {
    state->some_stat *= 1.05;
}

[SHOP_MY_ITEM] = {
    SHOP_MY_ITEM, "My Item", "Description of what it does.",
    base_cost, cost_scale, max_level,
    unlock_predicate, apply_my_item
},
```

The `apply` function is called once per purchase. `new_level` is the level after this purchase. Stateful effects (such as the Dedicated Crucible incrementing `crucibles_purchased`) belong here.

---

## Platform Notes

All platform-specific code is isolated in `include/ef_platform.h`:

| POSIX API | Windows equivalent | Notes |
|---|---|---|
| `mkdir(path, 0755)` | `_mkdir(path)` | Mode argument dropped on Windows |
| `rmdir(path)` | `_rmdir(path)` | |
| `stat` / `S_ISDIR` | `_stat` / `_S_IFDIR` | |
| `opendir` / `readdir` | `FindFirstFile` / `FindNextFile` | Minimal shim in `ef_platform.h` |
| `strcasecmp` | `_stricmp` | Aliased as `ef_strcasecmp` |
| `dirent.h` | built-in shim | `ef_platform.h` defines `DIR`, `struct dirent`, `opendir`, `readdir`, `closedir` for Windows |

SDL2 headers are included with the `__has_include` guard already in the codebase, so both `<SDL2/SDL.h>` (Linux/macOS) and `<SDL.h>` (Windows with MinGW SDL2 bundle) work without changes.
