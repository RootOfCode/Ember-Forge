#include "ef_prestige.h"

#include "ef_buildings.h"
#include "ef_format.h"
#include "ef_math.h"
#include "ef_resources.h"
#include "ef_state.h"
#include "ef_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef void (*EternalEffectFn)(GameState *state);

typedef struct EternalUpgradeDef {
  EternalUpgradeId id;
  const char *name;
  const char *description;
  double cost;
  EternalEffectFn effect;
} EternalUpgradeDef;

static void effect_apply_start_bonuses(GameState *state) {
  apply_eternal_start_bonuses(state);
}

static void effect_recompute_production(GameState *state) {
  recompute_production(state);
}

static const EternalUpgradeDef eternal_defs[ETERNAL_COUNT] = {
    [ETERNAL_MINER] = {ETERNAL_MINER, "Eternal Miner", "Start each run with 1 free Pickaxe.", 5.0, effect_apply_start_bonuses},
    [ETERNAL_EMBER_MEMORY] = {ETERNAL_EMBER_MEMORY, "Ember Memory", "Keep 10% of coins on Ascend.", 10.0, NULL},
    [ETERNAL_FORGE_MASTERY] = {ETERNAL_FORGE_MASTERY, "Forge Mastery", "All smelting is 25% faster permanently.", 25.0, NULL},
    [ETERNAL_ANCIENT_VEINS] = {ETERNAL_ANCIENT_VEINS, "Ancient Veins", "+50% all ore production permanently.", 50.0, effect_recompute_production},
    [ETERNAL_TWIN_FURNACE] = {ETERNAL_TWIN_FURNACE, "Twin Furnaces", "Start with Auto-Furnace already built.", 100.0, effect_apply_start_bonuses},
};

static const EternalUpgradeDef *eternal_def(EternalUpgradeId id) {
  if ((int)id < 0 || id >= ETERNAL_COUNT) {
    return NULL;
  }
  return &eternal_defs[id];
}

bool eternal_upgrade_owned(const GameState *state, EternalUpgradeId id) {
  return state && state->eternal_upgrades[id];
}

void apply_eternal_start_bonuses(GameState *state) {
  if (!state) {
    return;
  }
  if (eternal_upgrade_owned(state, ETERNAL_MINER)) {
    if (state->buildings[BLD_PICKAXE].count < 1) {
      state->buildings[BLD_PICKAXE].count = 1;
    }
  }
  if (eternal_upgrade_owned(state, ETERNAL_TWIN_FURNACE)) {
    if (state->buildings[BLD_FURNACE].count < 1) {
      state->buildings[BLD_FURNACE].count = 1;
    }
  }
}

bool buy_eternal_upgrade(GameState *state, EternalUpgradeId id) {
  if (!state) {
    return false;
  }
  const EternalUpgradeDef *def = eternal_def(id);
  if (!def) {
    return false;
  }
  if (eternal_upgrade_owned(state, id) || state->essence < def->cost) {
    return false;
  }
  state->essence -= def->cost;
  state->eternal_upgrades[id] = true;
  if (def->effect) {
    def->effect(state);
  }
  recompute_production(state);

  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Eternal: %s", def->name);
  notifs_add(state, msg, 3.0f, PAL_RED);
  return true;
}

bool prestige_available(const GameState *state) {
  return state && resource_ever(state, RES_EMBER) >= 1.0;
}

int essence_gain(const GameState *state) {
  if (!state) {
    return 0;
  }
  double ember = resource_ever(state, RES_EMBER);
  if (ember < 0.0) {
    ember = 0.0;
  }
  int base = (int)floor(sqrt(ember));
  int bonus = state->ascensions * 2;
  return base + bonus;
}

void recompute_prestige_mults(GameState *state) {
  if (!state) {
    return;
  }
  double e = state->essence;
  state->prod_mult = 1.0 + (e * 0.05);
  state->click_mult = 1.0 + (e * 0.02);
  state->gather_mult = 1.0 + (e * 0.015);
  state->smelt_speed_mult = 1.0 + (e * 0.01);
}

bool ascend(GameState *state) {
  if (!state || !prestige_available(state)) {
    return false;
  }

  int gain = essence_gain(state);
  double coins_before = resource_amount(state, RES_COINS);
  double kept_coins = eternal_upgrade_owned(state, ETERNAL_EMBER_MEMORY) ? (0.10 * coins_before) : 0.0;
  double start_coins = 25.0 + kept_coins;

  state->essence += (double)gain;
  state->ascensions += 1;
  recompute_prestige_mults(state);

  state->manual_mine_duration = 3.0f;
  state->manual_gather_duration = 3.0f;

  memset(state->resources, 0, sizeof(state->resources));
  memset(state->caps, 0, sizeof(state->caps));
  memset(state->ever_produced, 0, sizeof(state->ever_produced));
  memset(state->production, 0, sizeof(state->production));

  for (int i = 0; i < BLD_COUNT; ++i) {
    state->buildings[i] = (BuildingInstance){0, 1};
    state->building_mults[i] = 1.0;
  }

  memset(state->upgrades, 0, sizeof(state->upgrades));
  state->smelt_queue.len = 0;

  for (int i = 0; i < RES_COUNT; ++i) {
    state->unlocked[i] = false;
  }
  state->unlocked[RES_STONE] = true;
  state->unlocked[RES_COAL] = true;
  state->unlocked[RES_COINS] = true;

  state->smelt_slots_extra = 0;
  state->manual_mine.active = false;
  state->manual_gather.active = false;
  state->active_panel = PANEL_FORGE;

  state->active_event = EVENT_NONE;
  state->event_timer = 90.0f;
  state->smelt_boost_timer = 0.0f;
  state->iron_penalty_timer = 0.0f;

  state->sell_mult = 1.0;
  state->shop_discount = 1.0;
  memset(state->shop_purchases, 0, sizeof(state->shop_purchases));

  init_resource_caps(state);
  (void)resource_add(state, RES_COINS, start_coins);
  apply_eternal_start_bonuses(state);
  recompute_production(state);

  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Ascended (+%d Essence)", gain);
  notifs_add(state, msg, 3.0f, PAL_RED);
  return true;
}

