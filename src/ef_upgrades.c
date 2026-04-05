#include "ef_upgrades.h"
#include "ef_buildings.h"
#include "ef_content.h"
#include "ef_math.h"
#include "ef_state.h"
#include "ef_util.h"
#include <stdio.h>

static UpgradeDef upgrade_defs[UPG_COUNT];

#define UPGRADE(eid_, name_, desc_, unlock_fn, effect_fn, cost_spec)          \
    do {                                                                       \
        static const ResAmount _c[] = {cost_spec};                            \
        upgrade_defs[eid_] = (UpgradeDef){                                    \
            .id          = eid_,                                               \
            .name        = name_,                                              \
            .description = desc_,                                              \
            .cost        = _c,                                                 \
            .cost_count  = EF_ARRAY_COUNT(_c),                                \
            .effect      = effect_fn,                                          \
            .unlock      = unlock_fn,                                          \
        };                                                                     \
    } while (0)

/* ── unlock predicates ──────────────────────────────────────────────────── */
static bool ul_stone_20   (const GameState *s){ return s && resource_ever(s,RES_STONE)    >=20.0; }
static bool ul_stone_200  (const GameState *s){ return s && resource_ever(s,RES_STONE)    >=200.0; }
static bool ul_iron_bar_5 (const GameState *s){ return s && resource_ever(s,RES_IRON_BAR) >=5.0; }
static bool ul_iron_bar_20(const GameState *s){ return s && resource_ever(s,RES_IRON_BAR) >=20.0; }
static bool ul_steel_5    (const GameState *s){ return s && resource_ever(s,RES_STEEL)    >=5.0; }
static bool ul_steel_10   (const GameState *s){ return s && resource_ever(s,RES_STEEL)    >=10.0; }
static bool ul_steel_pl_5 (const GameState *s){ return s && resource_ever(s,RES_STEEL_PLATE)>=5.0; }
static bool ul_bronze_10  (const GameState *s){ return s && resource_ever(s,RES_BRONZE)   >=10.0; }
static bool ul_gear_10    (const GameState *s){ return s && resource_ever(s,RES_GEAR)     >=10.0; }
static bool ul_mpart_1    (const GameState *s){ return s && resource_ever(s,RES_MACHINE_PART)>=1.0; }
static bool ul_pick_2     (const GameState *s){ return s && building_count(s,BLD_PICKAXE)     >=2; }
static bool ul_pick_5     (const GameState *s){ return s && building_count(s,BLD_PICKAXE)     >=5; }
static bool ul_pick_20    (const GameState *s){ return s && building_count(s,BLD_PICKAXE)     >=20; }
static bool ul_coal_3     (const GameState *s){ return s && building_count(s,BLD_COAL_PIT)    >=3; }
static bool ul_coal_10    (const GameState *s){ return s && building_count(s,BLD_COAL_PIT)    >=10; }
static bool ul_furnace_1  (const GameState *s){ return s && building_count(s,BLD_FURNACE)     >=1; }
static bool ul_iron_mine_1(const GameState *s){ return s && building_count(s,BLD_IRON_MINE)   >=1; }
static bool ul_market_1   (const GameState *s){ return s && building_count(s,BLD_MARKET_STALL)>=1; }
static bool ul_market_2   (const GameState *s){ return s && building_count(s,BLD_MARKET_STALL)>=2; }
static bool ul_bellows_1  (const GameState *s){ return s && building_count(s,BLD_BELLOWS_SHOP)>=1; }
static bool ul_mythril_1  (const GameState *s){ return s && building_count(s,BLD_MYTHRIL_DRILL)>=1; }
static bool ul_slot_1     (const GameState *s){ return s && s->upgrades[UPG_SMELT_SLOT_1]; }

/* ── effect functions ───────────────────────────────────────────────────── */
static void fx_click_x2   (GameState *s){ s->click_mult  *= 2.0; }
static void fx_gather_x2  (GameState *s){ s->gather_mult *= 2.0; }
static void fx_quick_hands(GameState *s){
    s->manual_mine_duration   = clamp_float(s->manual_mine_duration   - 0.5f,0.5f,10000.0f);
    s->manual_gather_duration = clamp_float(s->manual_gather_duration - 0.5f,0.5f,10000.0f);
}
static void fx_better_picks  (GameState *s){ multiply_building_prod(s,BLD_PICKAXE,     2.0); }
static void fx_coal_veins    (GameState *s){ multiply_building_prod(s,BLD_COAL_PIT,    1.5); }
static void fx_smelt_slot    (GameState *s){ s->smelt_slots_extra += 1; }
static void fx_smelt_tuning  (GameState *s){ s->smelt_speed_mult  *= 1.25; }
static void fx_market_network(GameState *s){ multiply_building_prod(s,BLD_MARKET_STALL,2.0); }
static void fx_ancient_veins (GameState *s){
    multiply_building_prod(s,BLD_IRON_MINE,    1.5);
    multiply_building_prod(s,BLD_COPPER_MINE,  1.5);
    multiply_building_prod(s,BLD_TINSMITH,     1.5);
    multiply_building_prod(s,BLD_MYTHRIL_DRILL,1.5);
}
static void fx_rein_sacks(GameState *s){
    s->caps[RES_STONE] = resource_cap(s,RES_STONE) * 1.5;
    s->caps[RES_COAL]  = resource_cap(s,RES_COAL)  * 1.5;
}
static void fx_ore_crates(GameState *s){
    s->caps[RES_IRON]   = resource_cap(s,RES_IRON)   * 1.5;
    s->caps[RES_COPPER] = resource_cap(s,RES_COPPER) * 1.5;
    s->caps[RES_TIN]    = resource_cap(s,RES_TIN)    * 1.5;
}
static void fx_mkt_signage(GameState *s)  { multiply_building_prod(s,BLD_MARKET_STALL, 1.5); }
static void fx_ind_tools  (GameState *s)  { multiply_building_prod(s,BLD_MYTHRIL_DRILL,1.5); }
static void fx_gear_picks (GameState *s)  { multiply_building_prod(s,BLD_PICKAXE,      1.75); }
static void fx_coal_filt  (GameState *s)  { multiply_building_prod(s,BLD_COAL_PIT,     1.75); }
static void fx_ore_sort   (GameState *s){
    multiply_building_prod(s,BLD_IRON_MINE,  1.4);
    multiply_building_prod(s,BLD_COPPER_MINE,1.4);
    multiply_building_prod(s,BLD_TINSMITH,   1.4);
}
static void fx_assembly   (GameState *s)  { s->prod_mult *= 1.15; }
static void fx_mech_drills(GameState *s)  { multiply_building_prod(s,BLD_MYTHRIL_DRILL,2.0); }

/* ── upgrades_init — declare all upgrades here ───────────────────────────
 * To add: enum in ef_defs.h, key in ef_defs.c, call here. */
void upgrades_init(void) {
    UPGRADE(UPG_STURDY_GLOVES,   "Sturdy Gloves",       "Double click mining power.",
        ul_stone_20,   fx_click_x2,    COSTS({RES_COINS,25.0},{RES_STONE,20.0}));
    UPGRADE(UPG_MINERS_POUCH,    "Miner's Pouch",       "Double manual gathering yield.",
        ul_stone_20,   fx_gather_x2,   COSTS({RES_COINS,25.0},{RES_STONE,20.0}));
    UPGRADE(UPG_QUICK_HANDS,     "Quick Hands",         "Manual actions 0.5s faster.",
        ul_pick_2,     fx_quick_hands, COSTS({RES_COINS,60.0},{RES_STONE,50.0}));
    UPGRADE(UPG_BETTER_PICKS,    "Hardened Pickaxes",   "Pickaxes mine 2x faster.",
        ul_pick_5,     fx_better_picks,COSTS({RES_COINS,100.0},{RES_IRON_BAR,5.0}));
    UPGRADE(UPG_COAL_VEINS,      "Deep Coal Seams",     "Coal pits +50% coal.",
        ul_coal_3,     fx_coal_veins,  COSTS({RES_COINS,300.0},{RES_STONE,50.0}));
    UPGRADE(UPG_STEEL_ALLOY,     "Steel Alloy Research","Unlocks steel smelting recipe.",
        ul_iron_bar_20,NULL,           COSTS({RES_COINS,2000.0},{RES_IRON_BAR,20.0},{RES_BRONZE,10.0}));
    UPGRADE(UPG_SMELT_SLOT_1,    "Second Crucible",     "Adds one extra smelting slot.",
        ul_furnace_1,  fx_smelt_slot,  COSTS({RES_COINS,800.0},{RES_BRONZE,5.0}));
    UPGRADE(UPG_SMELT_SLOT_2,    "Third Crucible",      "Adds one extra smelting slot.",
        ul_slot_1,     fx_smelt_slot,  COSTS({RES_COINS,2500.0},{RES_STEEL,5.0}));
    UPGRADE(UPG_SMELT_TUNING,    "Furnace Tuning",      "Smelting is 25% faster.",
        ul_iron_bar_5, fx_smelt_tuning,COSTS({RES_COINS,500.0},{RES_IRON_BAR,10.0}));
    UPGRADE(UPG_MARKET_NETWORK,  "Market Network",      "Market stalls 2x coins.",
        ul_market_1,   fx_market_network,COSTS({RES_COINS,600.0},{RES_COPPER_BAR,10.0}));
    UPGRADE(UPG_DEEP_DRILLING,   "Deep-Core Drilling",  "Unlocks Mythril Drill.",
        ul_steel_10,   NULL,           COSTS({RES_COINS,20000.0},{RES_STEEL,20.0},{RES_GEAR,10.0}));
    UPGRADE(UPG_ANCIENT_VEINS,   "Ancient Veins",       "+50% ore production permanently.",
        ul_steel_5,    fx_ancient_veins,COSTS({RES_COINS,8000.0},{RES_STEEL,10.0},{RES_BRONZE,20.0}));
    UPGRADE(UPG_REINFORCED_SACKS,"Reinforced Sacks",    "Stone/Coal storage +50%.",
        ul_stone_200,  fx_rein_sacks,  COSTS({RES_COINS,120.0},{RES_STONE,200.0},{RES_IRON_BAR,5.0}));
    UPGRADE(UPG_ORE_CRATES,      "Ore Crates",          "Ore storage +50%.",
        ul_iron_mine_1,fx_ore_crates,  COSTS({RES_COINS,260.0},{RES_GEAR,5.0},{RES_BRONZE,5.0}));
    UPGRADE(UPG_MARKET_SIGNAGE,  "Bright Signage",      "Market stalls +50% coins.",
        ul_market_1,   fx_mkt_signage, COSTS({RES_COINS,350.0},{RES_COPPER_BAR,10.0},{RES_STONE,150.0}));
    UPGRADE(UPG_PRECISION_MOLDS, "Precision Molds",     "Gears/bellows/rivets 2x output.",
        ul_gear_10,    NULL,           COSTS({RES_COINS,900.0},{RES_BRONZE,20.0},{RES_IRON_BAR,20.0}));
    UPGRADE(UPG_METALWORKING,    "Metalworking",        "Unlocks Steel Plates & Rivets.",
        ul_steel_5,    NULL,           COSTS({RES_COINS,1800.0},{RES_STEEL,5.0},{RES_BRONZE,10.0}));
    UPGRADE(UPG_COIN_MINTING,    "Coin Minting",        "Unlocks Trade Token recipe.",
        ul_market_2,   NULL,           COSTS({RES_COINS,2500.0},{RES_BRONZE,25.0},{RES_GEAR,15.0}));
    UPGRADE(UPG_INDUSTRIAL_TOOLS,"Industrial Tools",    "Machine Parts; Mythril Drills +50%.",
        ul_steel_pl_5, fx_ind_tools,   COSTS({RES_COINS,15000.0},{RES_STEEL_PLATE,10.0},{RES_GEAR,20.0},{RES_RIVET,50.0}));
    UPGRADE(UPG_GEAR_DRIVEN_PICKS,"Gear-Driven Picks",  "Pickaxes mine 75% faster.",
        ul_pick_20,    fx_gear_picks,  COSTS({RES_COINS,1200.0},{RES_GEAR,50.0}));
    UPGRADE(UPG_COAL_DUST_FILTERS,"Coal Dust Filters",  "Coal pits +75% coal.",
        ul_coal_10,    fx_coal_filt,   COSTS({RES_COINS,900.0},{RES_BRONZE,12.0},{RES_GEAR,6.0}));
    UPGRADE(UPG_ORE_SORTING,     "Ore Sorting",         "Iron/Copper/Tin +40%.",
        ul_bronze_10,  fx_ore_sort,    COSTS({RES_COINS,3000.0},{RES_BRONZE,20.0},{RES_GEAR,10.0}));
    UPGRADE(UPG_BELLOWS_EFFICIENCY,"Bellows Efficiency","Bellows Workshops 15% speed each.",
        ul_bellows_1,  NULL,           COSTS({RES_COINS,5000.0},{RES_BELLOWS,10.0},{RES_STEEL,10.0}));
    UPGRADE(UPG_ASSEMBLY_LINES,  "Assembly Lines",      "All building production +15%.",
        ul_mpart_1,    fx_assembly,    COSTS({RES_COINS,8000.0},{RES_MACHINE_PART,3.0},{RES_RIVET,150.0},{RES_STEEL_PLATE,15.0}));
    UPGRADE(UPG_MECHANIZED_DRILLS,"Mechanized Drills",  "Mythril Drills 2x mythril.",
        ul_mythril_1,  fx_mech_drills, COSTS({RES_COINS,75000.0},{RES_MACHINE_PART,8.0},{RES_STEEL,80.0}));
}

#undef UPGRADE

/* ── accessor API ───────────────────────────────────────────────────────── */
const UpgradeDef *upgrade_def(UpgradeId id) {
    if ((int)id < 0 || id >= UPG_COUNT) return NULL;
    return &upgrade_defs[id];
}
const char *upgrade_name(UpgradeId id) {
    const UpgradeDef *d = upgrade_def(id); return d ? d->name : "???";
}
const char *upgrade_description(UpgradeId id) {
    const UpgradeDef *d = upgrade_def(id); return d ? d->description : "";
}
bool upgrade_owned(const GameState *state, UpgradeId id) {
    return state && state->upgrades[id];
}
bool upgrade_available(const GameState *state, const UpgradeDef *def) {
    if (!state || !def || upgrade_owned(state, def->id)) return false;
    if (!def->unlock) return true;
    return def->unlock(state);
}
bool buy_upgrade(GameState *state, UpgradeId id) {
    if (!state) return false;
    const UpgradeDef *def = upgrade_def(id);
    if (!def || !upgrade_available(state, def)) return false;
    if (!can_afford_list(state, def->cost, def->cost_count)) return false;
    spend_list(state, def->cost, def->cost_count);
    state->upgrades[id] = true;
    if (def->effect) def->effect(state);
    recompute_production(state);
    char msg[256];
    (void)snprintf(msg, sizeof(msg), "Upgrade: %s", def->name);
    notifs_add(state, msg, 3.0f, PAL_GOLD);
    return true;
}
