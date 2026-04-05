#include "ef_buildings.h"
#include "ef_content.h"
#include "ef_format.h"
#include "ef_math.h"
#include "ef_state.h"
#include "ef_util.h"
#include <math.h>
#include <stdio.h>

static BuildingDef building_defs[BLD_COUNT];

/*
 * DSL macros — the eid_ parameter avoids clashing with the .id struct field.
 * BUILDING  : building with automatic per-second production.
 * BUILDING_NP: No Production — effect handled elsewhere (smelt slots, speed).
 */
#define BUILDING(eid_, name_, desc_, scale_, unlock_, cost_spec, prod_spec)   \
    do {                                                                       \
        static const ResAmount _c[] = {cost_spec};                            \
        static const ResAmount _p[] = {prod_spec};                            \
        building_defs[eid_] = (BuildingDef){                                  \
            .id               = eid_,                                          \
            .name             = name_,                                         \
            .description      = desc_,                                         \
            .base_cost        = _c,                                            \
            .base_cost_count  = EF_ARRAY_COUNT(_c),                           \
            .cost_scale       = scale_,                                        \
            .production       = _p,                                            \
            .production_count = EF_ARRAY_COUNT(_p),                           \
            .unlock           = unlock_,                                       \
        };                                                                     \
    } while (0)

#define BUILDING_NP(eid_, name_, desc_, scale_, unlock_, cost_spec)           \
    do {                                                                       \
        static const ResAmount _c[] = {cost_spec};                            \
        building_defs[eid_] = (BuildingDef){                                  \
            .id               = eid_,                                          \
            .name             = name_,                                         \
            .description      = desc_,                                         \
            .base_cost        = _c,                                            \
            .base_cost_count  = EF_ARRAY_COUNT(_c),                           \
            .cost_scale       = scale_,                                        \
            .production       = NULL,                                          \
            .production_count = 0,                                             \
            .unlock           = unlock_,                                       \
        };                                                                     \
    } while (0)

/* ── unlock predicates ──────────────────────────────────────────────────── */
static bool unlock_iron_mine    (const GameState *s){ return s && resource_ever(s, RES_STONE) >= 50.0; }
static bool unlock_copper_mine  (const GameState *s){ return s && resource_ever(s, RES_STONE) >= 50.0; }
static bool unlock_furnace      (const GameState *s){ return s && building_count(s, BLD_IRON_MINE) >= 1; }
static bool unlock_market_stall (const GameState *s){ return s && resource_ever(s, RES_STONE) >= 100.0; }
static bool unlock_mythril_drill(const GameState *s){ return s && s->upgrades[UPG_DEEP_DRILLING]; }

/* ── buildings_init — declare all buildings here ─────────────────────────
 * To add a building: add enum to ef_defs.h, key to ef_defs.c, call here. */
void buildings_init(void) {
    BUILDING(BLD_PICKAXE, "Rusty Pickaxe",
        "Mines stone automatically. Slow, but it never sleeps.",
        1.15, NULL,
        COSTS({RES_COINS, 10.0}),
        PRODS({RES_STONE, 0.5}));

    BUILDING(BLD_COAL_PIT, "Coal Pit",
        "Digs coal from the earth. Requires stone infrastructure.",
        1.15, NULL,
        COSTS({RES_COINS, 50.0}, {RES_STONE, 20.0}),
        PRODS({RES_COAL, 0.3}));

    BUILDING(BLD_IRON_MINE, "Iron Mine",
        "Extracts iron ore from deep veins.",
        1.15, unlock_iron_mine,
        COSTS({RES_COINS, 200.0}, {RES_STONE, 100.0}),
        PRODS({RES_IRON, 0.2}));

    BUILDING(BLD_COPPER_MINE, "Copper Mine",
        "Extracts copper ore.",
        1.15, unlock_copper_mine,
        COSTS({RES_COINS, 250.0}, {RES_STONE, 120.0}),
        PRODS({RES_COPPER, 0.2}));

    BUILDING(BLD_TINSMITH, "Tinsmith",
        "Refines tin for alloying.",
        1.15, NULL,
        COSTS({RES_COINS, 400.0}, {RES_COPPER_BAR, 5.0}),
        PRODS({RES_TIN, 0.15}));

    BUILDING_NP(BLD_FURNACE, "Auto-Furnace",
        "Automatically runs iron bar smelt jobs; adds one smelting slot.",
        1.15, unlock_furnace,
        COSTS({RES_COINS, 500.0}, {RES_STONE, 200.0}, {RES_GEAR, 5.0}));

    BUILDING_NP(BLD_BELLOWS_SHOP, "Bellows Workshop",
        "Speeds up all smelting by 10% per workshop.",
        1.15, NULL,
        COSTS({RES_COINS, 1000.0}, {RES_BRONZE, 10.0}, {RES_BELLOWS, 3.0}));

    BUILDING(BLD_MARKET_STALL, "Market Stall",
        "Generates coins from trade and scrap.",
        1.15, unlock_market_stall,
        COSTS({RES_COINS, 80.0}, {RES_STONE, 60.0}, {RES_COAL, 20.0}),
        PRODS({RES_COINS, 0.35}));

    BUILDING(BLD_MYTHRIL_DRILL, "Mythril Drill",
        "Bores deep into ancient rock for mythril.",
        1.15, unlock_mythril_drill,
        COSTS({RES_COINS, 50000.0}, {RES_STEEL, 50.0}, {RES_GEAR, 30.0}),
        PRODS({RES_MYTHRIL, 0.05}));
}

#undef BUILDING
#undef BUILDING_NP

/* ── accessor API ───────────────────────────────────────────────────────── */
const BuildingDef *building_def(BuildingId id) {
    if ((int)id < 0 || id >= BLD_COUNT) return NULL;
    return &building_defs[id];
}
const char *building_name(BuildingId id) {
    const BuildingDef *d = building_def(id); return d ? d->name : "???";
}
const char *building_description(BuildingId id) {
    const BuildingDef *d = building_def(id); return d ? d->description : "";
}

/* ── game logic ─────────────────────────────────────────────────────────── */
int building_count(const GameState *state, BuildingId id) {
    if (!state || (int)id < 0 || id >= BLD_COUNT) return 0;
    return state->buildings[id].count;
}
double building_prod_mult(const GameState *state, BuildingId id) {
    if (!state || (int)id < 0 || id >= BLD_COUNT) return 1.0;
    double base  = state->building_mults[id];
    double veins = 1.0;
    if (state->eternal_upgrades[ETERNAL_ANCIENT_VEINS])
        if (id==BLD_IRON_MINE || id==BLD_COPPER_MINE ||
            id==BLD_TINSMITH   || id==BLD_MYTHRIL_DRILL)
            veins = 1.5;
    return base * veins;
}
void multiply_building_prod(GameState *state, BuildingId id, double factor) {
    if (!state || (int)id < 0 || id >= BLD_COUNT) return;
    state->building_mults[id] = building_prod_mult(state, id) * factor;
}
bool building_unlocked(const GameState *state, const BuildingDef *def) {
    if (!state || !def) return false;
    if (building_count(state, def->id) > 0) return true;
    if (def->unlock) return def->unlock(state);
    return true;
}
size_t building_cost(BuildingId id, int owned, ResAmount *out, size_t cap) {
    const BuildingDef *def = building_def(id);
    if (!def || !out || cap == 0) return 0;
    double scale = def->cost_scale > 0.0 ? def->cost_scale : 1.15;
    size_t n = 0;
    for (size_t i = 0; i < def->base_cost_count && n < cap; ++i)
        out[n++] = (ResAmount){def->base_cost[i].res,
                               def->base_cost[i].amount * pow(scale, (double)owned)};
    return n;
}
bool buy_building(GameState *state, BuildingId id) {
    if (!state) return false;
    const BuildingDef *def = building_def(id);
    if (!def || !building_unlocked(state, def)) return false;
    ResAmount buf[8];
    size_t cn = building_cost(id, building_count(state, id), buf, EF_ARRAY_COUNT(buf));
    if (!can_afford_list(state, buf, cn)) return false;
    spend_list(state, buf, cn);
    state->buildings[id].count += 1;
    recompute_production(state);
    char msg[256];
    (void)snprintf(msg, sizeof(msg), "Built: %s", def->name);
    notifs_add(state, msg, 3.0f, PAL_GREEN_OK);
    return true;
}
void recompute_production(GameState *state) {
    if (!state) return;
    for (int i = 0; i < RES_COUNT; ++i) state->production[i] = 0.0;
    for (int i = 0; i < BLD_COUNT; ++i) {
        const BuildingDef *def = &building_defs[i];
        int n = building_count(state, def->id);
        if (n <= 0 || def->production_count == 0) continue;
        double mult = building_prod_mult(state, def->id) * state->prod_mult;
        for (size_t j = 0; j < def->production_count; ++j)
            state->production[def->production[j].res] +=
                (double)n * def->production[j].amount * mult;
    }
}
void tick_buildings(GameState *state, float dt_seconds) {
    if (!state) return;
    double iron_mult = state->iron_penalty_timer > 0.0f ? 0.9 : 1.0;
    double ddt = (double)dt_seconds;
    for (int i = 0; i < RES_COUNT; ++i) {
        double rate = state->production[i];
        if (rate == 0.0) continue;
        double add = rate * ddt;
        if ((ResourceId)i == RES_IRON) add *= iron_mult;
        (void)resource_add(state, (ResourceId)i, add);
    }
}
int max_smelt_slots(const GameState *state) {
    if (!state) return 5;
    return 5 + state->smelt_slots_extra + building_count(state, BLD_FURNACE);
}
double smelt_speed_multiplier(const GameState *state) {
    if (!state) return 1.0;
    double pulse   = state->smelt_boost_timer > 0.0f ? 2.0 : 1.0;
    double mastery = state->eternal_upgrades[ETERNAL_FORGE_MASTERY] ? 1.25 : 1.0;
    double per_ws  = state->upgrades[UPG_BELLOWS_EFFICIENCY] ? 0.15 : 0.10;
    double shop    = 1.0 + per_ws * (double)building_count(state, BLD_BELLOWS_SHOP);
    return state->smelt_speed_mult * pulse * mastery * shop;
}
