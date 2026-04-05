/*
 * ef_content.h — Game Content DSL
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  HOW TO DECLARE A NEW BUILDING
 * ────────────────────────────────────────────────────────────────────────────
 *       BUILDING(BLD_MY_THING, "My Thing", "Desc.",
 *           1.15, my_unlock_fn,
 *           COSTS({RES_COINS, 100.0}, {RES_IRON, 5.0}),
 *           PRODS({RES_STEEL, 0.1}));
 *
 *       BUILDING_NP(BLD_MY_THING, "No-production thing.", "Desc.",
 *           1.15, my_unlock_fn,
 *           COSTS({RES_COINS, 500.0}));
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  HOW TO DECLARE A NEW UPGRADE
 * ────────────────────────────────────────────────────────────────────────────
 *    UPGRADE(UPG_MY_UPGRADE, "My Upgrade", "Desc.",
 *        my_unlock_fn, my_effect_fn,
 *        COSTS({RES_COINS, 250.0}, {RES_BRONZE, 10.0}));
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  HOW TO DECLARE A NEW RECIPE
 * ────────────────────────────────────────────────────────────────────────────
 *    RECIPE(RCP_MY_RECIPE, "My Recipe",
 *        my_unlock_fn, 8.0f,
 *        INPUTS({RES_IRON, 2.0}, {RES_COAL, 1.0}),
 *        OUTPUTS({RES_IRON_BAR, 1.0}));
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  RESOURCE TABLE  (EF_EACH_RESOURCE X-macro)
 * ────────────────────────────────────────────────────────────────────────────
 *  Include this file and expand EF_EACH_RESOURCE(X) to generate per-resource
 *  code:
 *
 *    #define X(id, key, name, color, tier, base_cap, sell, gather_time) ...
 *    EF_EACH_RESOURCE(X)
 *    #undef X
 *
 *   id          — ResourceId enum value
 *   key         — stable save-file string  (no spaces, no colons)
 *   name        — display name shown in the UI
 *   color       — PaletteColor for rendering
 *   tier        — resource tier (affects unlock ordering)
 *   base_cap    — starting storage cap
 *   sell_value  — coins per unit  (0.0 = not sellable)
 *   gather_time — base seconds for one manual gather action.
 *                 Stone is the easiest (shortest); rarer materials take
 *                 progressively longer.  The actual duration is divided by
 *                 the player's gather_mult, so upgrades still help.
 *                 Resources that cannot be gathered manually (bars, coins,
 *                 processed goods) use 0.0 — the Mine panel hides them.
 */

#pragma once

#include "ef_defs.h"
#include "ef_palette.h"

/* ── ResAmount list helpers ─────────────────────────────────────────────── */
#define COSTS(...)   __VA_ARGS__
#define PRODS(...)   __VA_ARGS__
#define INPUTS(...)  __VA_ARGS__
#define OUTPUTS(...) __VA_ARGS__

/* ── Resource table ─────────────────────────────────────────────────────────
 *
 *  Columns: id, key, name, color, tier, base_cap, sell_value, gather_time
 *
 *  gather_time (seconds):
 *    Stone   1.5  — dirt-common, the first thing you ever click
 *    Coal    2.5  — slightly deeper, burns your fingers
 *    Iron    3.5  — heavy ore, real effort to chisel out
 *    Copper  3.5  — similar depth to iron
 *    Tin     4.5  — rarer seams, slower work
 *    Mythril 8.0  — ancient rock, gruelling to extract manually
 *    0.0     —— not manually gatherable (bars, coins, processed goods)
 */
#define EF_EACH_RESOURCE(X)                                                                               \
/*   id              key            name            color       tier  cap     sell   gather_time */        \
    X(RES_STONE,       "stone",       "Stone",        PAL_GRAY,    0,  500.0,   0.5,   4.0)               \
    X(RES_COAL,        "coal",        "Coal",         PAL_DARK,    0,  300.0,   0.8,   6.0)               \
    X(RES_COINS,       "coins",       "Coins",        PAL_GOLD,    0,  1.0e6,   0.0,   0.0)               \
    X(RES_IRON,        "iron",        "Iron Ore",     PAL_RUST,    1,  200.0,   1.2,   8.0)               \
    X(RES_COPPER,      "copper",      "Copper Ore",   PAL_ORANGE,  1,  200.0,   1.2,   8.0)               \
    X(RES_TIN,         "tin",         "Tin Ore",      PAL_SILVER,  1,  200.0,   1.5,  10.0)               \
    X(RES_IRON_BAR,    "iron_bar",    "Iron Bar",     PAL_SILVER,  2,  100.0,   5.0,   0.0)               \
    X(RES_COPPER_BAR,  "copper_bar",  "Copper Bar",   PAL_ORANGE,  2,  100.0,   5.0,   0.0)               \
    X(RES_BRONZE,      "bronze",      "Bronze Bar",   PAL_BRONZE,  3,   80.0,  10.0,   0.0)               \
    X(RES_GEAR,        "gear",        "Gear",         PAL_GRAY,    3,  200.0,   8.0,   0.0)               \
    X(RES_BELLOWS,     "bellows",     "Bellows",      PAL_BROWN,   3,   50.0,  25.0,   0.0)               \
    X(RES_STEEL,       "steel",       "Steel Bar",    PAL_BLUE,    4,   60.0,  20.0,   0.0)               \
    X(RES_STEEL_PLATE, "steel_plate", "Steel Plate",  PAL_BLUE,    4,   80.0,  35.0,   0.0)               \
    X(RES_RIVET,       "rivet",       "Rivets",       PAL_SILVER,  3,  400.0,   2.0,   0.0)               \
    X(RES_MACHINE_PART,"machine_part","Machine Part", PAL_GRAY,    5,   50.0, 120.0,   0.0)               \
    X(RES_MYTHRIL,     "mythril",     "Mythril Ore",  PAL_CYAN,    5,   40.0,  50.0,  18.0)               \
    X(RES_EMBER,       "ember",       "Ember Crystal",PAL_RED,     6,   10.0, 200.0,   0.0)
