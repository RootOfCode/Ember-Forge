/*
 * ef_defs.c — Enum ↔ key-string mappings
 *
 * All key strings are plain C identifiers (lowercase, underscores).
 * No Lisp colons, no hyphens.  The resource table is derived from the
 * single source of truth in ef_content.h so adding a resource only
 * requires one change there.
 */
#include "ef_defs.h"
#include "ef_content.h"

#include <string.h>

/* ── generic linear search ──────────────────────────────────────────────── */
static bool key_lookup(const char *key, const char *const *table,
                        size_t table_len, int *out_index) {
    if (!key || !out_index) return false;
    for (size_t i = 0; i < table_len; ++i)
        if (table[i] && strcmp(table[i], key) == 0) {
            *out_index = (int)i;
            return true;
        }
    return false;
}

/* ── Panel ──────────────────────────────────────────────────────────────── */
static const char *const panel_keys[PANEL_COUNT] = {
    "forge", "mine", "upgrades", "recipes", "shop", "prestige"
};

const char *panel_key(Panel p) {
    return ((int)p >= 0 && p < PANEL_COUNT) ? panel_keys[p] : "forge";
}
bool panel_from_key(const char *key, Panel *out) {
    int idx = 0;
    if (!key_lookup(key, panel_keys, PANEL_COUNT, &idx) || !out) return false;
    *out = (Panel)idx; return true;
}

/* ── SmeltSource ────────────────────────────────────────────────────────── */
static const char *const smelt_source_keys[2] = {"manual", "auto"};

const char *smelt_source_key(SmeltSource s) {
    return (s == SMELT_SOURCE_AUTO) ? smelt_source_keys[1] : smelt_source_keys[0];
}
bool smelt_source_from_key(const char *key, SmeltSource *out) {
    int idx = 0;
    if (!key_lookup(key, smelt_source_keys, 2, &idx) || !out) return false;
    *out = (SmeltSource)idx; return true;
}

/* ── Resource ───────────────────────────────────────────────────────────── *
 * Built from EF_EACH_RESOURCE so it stays in sync with ef_content.h.       */
#define X(id, key, name, color, tier, cap, sell, gather_time)  [id] = key,
static const char *const resource_keys[RES_COUNT] = { EF_EACH_RESOURCE(X) };
#undef X

const char *resource_key(ResourceId r) {
    return ((int)r >= 0 && r < RES_COUNT) ? resource_keys[r] : "stone";
}
bool resource_from_key(const char *key, ResourceId *out) {
    int idx = 0;
    if (!key_lookup(key, resource_keys, RES_COUNT, &idx) || !out) return false;
    *out = (ResourceId)idx; return true;
}

/* ── Building ───────────────────────────────────────────────────────────── */
static const char *const building_keys[BLD_COUNT] = {
    "pickaxe", "coal_pit", "iron_mine", "copper_mine", "tinsmith",
    "furnace", "bellows_shop", "market_stall", "mythril_drill"
};

const char *building_key(BuildingId b) {
    return ((int)b >= 0 && b < BLD_COUNT) ? building_keys[b] : "pickaxe";
}
bool building_from_key(const char *key, BuildingId *out) {
    int idx = 0;
    if (!key_lookup(key, building_keys, BLD_COUNT, &idx) || !out) return false;
    *out = (BuildingId)idx; return true;
}

/* ── Recipe ─────────────────────────────────────────────────────────────── */
static const char *const recipe_keys[RCP_COUNT] = {
    "iron_bar", "copper_bar", "bronze", "steel", "gear", "bellows_part",
    "machined_gears", "rivets", "steel_plate", "machine_part",
    "coin_mint", "ember_crystal"
};

const char *recipe_key(RecipeId r) {
    return ((int)r >= 0 && r < RCP_COUNT) ? recipe_keys[r] : "iron_bar";
}
bool recipe_from_key(const char *key, RecipeId *out) {
    int idx = 0;
    if (!key_lookup(key, recipe_keys, RCP_COUNT, &idx) || !out) return false;
    *out = (RecipeId)idx; return true;
}

/* ── Upgrade ────────────────────────────────────────────────────────────── */
static const char *const upgrade_keys[UPG_COUNT] = {
    "sturdy_gloves",    "miners_pouch",       "quick_hands",       "better_picks",
    "coal_veins",       "steel_alloy",        "smelt_slot_1",      "smelt_slot_2",
    "smelt_tuning",     "market_network",     "deep_drilling",     "ancient_veins",
    "reinforced_sacks", "ore_crates",         "market_signage",    "precision_molds",
    "metalworking",     "coin_minting",       "industrial_tools",  "gear_driven_picks",
    "coal_dust_filters","ore_sorting",        "bellows_efficiency","assembly_lines",
    "mechanized_drills"
};

const char *upgrade_key(UpgradeId u) {
    return ((int)u >= 0 && u < UPG_COUNT) ? upgrade_keys[u] : "sturdy_gloves";
}
bool upgrade_from_key(const char *key, UpgradeId *out) {
    int idx = 0;
    if (!key_lookup(key, upgrade_keys, UPG_COUNT, &idx) || !out) return false;
    *out = (UpgradeId)idx; return true;
}

/* ── Shop item ──────────────────────────────────────────────────────────── */
static const char *const shop_item_keys[SHOP_COUNT] = {
    "haggling", "salesmanship", "warehouse", "smelters_coupon", "foreman_contract",
    "dedicated_crucible"
};

const char *shop_item_key(ShopItemId s) {
    return ((int)s >= 0 && s < SHOP_COUNT) ? shop_item_keys[s] : "haggling";
}
bool shop_item_from_key(const char *key, ShopItemId *out) {
    int idx = 0;
    if (!key_lookup(key, shop_item_keys, SHOP_COUNT, &idx) || !out) return false;
    *out = (ShopItemId)idx; return true;
}

/* ── Eternal upgrade ────────────────────────────────────────────────────── */
static const char *const eternal_upgrade_keys[ETERNAL_COUNT] = {
    "eternal_miner", "ember_memory", "forge_mastery",
    "eternal_ancient_veins", "twin_furnace"
};

const char *eternal_upgrade_key(EternalUpgradeId e) {
    return ((int)e >= 0 && e < ETERNAL_COUNT) ? eternal_upgrade_keys[e] : "eternal_miner";
}
bool eternal_upgrade_from_key(const char *key, EternalUpgradeId *out) {
    int idx = 0;
    if (!key_lookup(key, eternal_upgrade_keys, ETERNAL_COUNT, &idx) || !out) return false;
    *out = (EternalUpgradeId)idx; return true;
}

/* ── Event ──────────────────────────────────────────────────────────────── */
static const char *const event_keys[EVENT_COUNT] = {
    "cave_in", "wandering_merchant", "ember_pulse", "scrap_windfall", "ancient_cache"
};

const char *event_key(EventId e) {
    return (e > EVENT_NONE && e < EVENT_COUNT) ? event_keys[e] : "cave_in";
}
bool event_from_key(const char *key, EventId *out) {
    int idx = 0;
    if (!key_lookup(key, event_keys, EVENT_COUNT, &idx) || !out) return false;
    *out = (EventId)idx; return true;
}

/* ── Achievement ────────────────────────────────────────────────────────── */
static const char *const achievement_keys[ACH_COUNT] = {
    "first_spark",    "iron_will",       "the_alchemist",   "deep_echo",
    "ember_awakened", "eternal_flame",   "stone_stockpile", "coal_baron",
    "pickaxe_army",   "furnace_fleet",   "bellows_business","merchant_prince",
    "gearhead",       "airflow_artisan", "bronze_age",      "steelworks",
    "plate_press",    "rivet_rain",      "parts_bin",       "shopaholic",
    "queue_master",   "tinkerer",        "industrial_revolution"
};

const char *achievement_key(AchievementId a) {
    return ((int)a >= 0 && a < ACH_COUNT) ? achievement_keys[a] : "first_spark";
}
bool achievement_from_key(const char *key, AchievementId *out) {
    int idx = 0;
    if (!key_lookup(key, achievement_keys, ACH_COUNT, &idx) || !out) return false;
    *out = (AchievementId)idx; return true;
}
