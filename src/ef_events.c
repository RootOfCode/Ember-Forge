#include "ef_events.h"

#include "ef_buildings.h"
#include "ef_resources.h"
#include "ef_state.h"
#include "ef_upgrades.h"
#include "ef_util.h"

#include <stdlib.h>
#include <stdio.h>

static void schedule_event_timer(GameState *state, float seconds) {
  if (!state) {
    return;
  }
  state->event_timer = seconds;
}

void schedule_next_event(GameState *state) {
  if (!state) {
    return;
  }
  int r = rand() % 600;
  schedule_event_timer(state, (float)(300 + r));
}

static void notify_event(GameState *state, const char *name) {
  if (!state || !name) {
    return;
  }
  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Event: %s", name);
  notifs_add(state, msg, 3.0f, PAL_WARN);
}

static bool can_stone_50(const GameState *state) {
  return state && resource_has(state, RES_STONE, 50.0);
}

static void effect_cave_in_shore(GameState *state) {
  (void)resource_sub(state, RES_STONE, 50.0);
  notifs_add(state, "You reinforce the supports. Safe again.", 3.0f, PAL_GREEN_OK);
}

static void effect_cave_in_abandon(GameState *state) {
  state->iron_penalty_timer = 60.0f;
  notifs_add(state, "Iron production reduced temporarily.", 3.0f, PAL_WARN);
}

static bool can_steel_5(const GameState *state) {
  return state && resource_has(state, RES_STEEL, 5.0);
}

static void effect_merchant_trade_steel(GameState *state) {
  (void)resource_sub(state, RES_STEEL, 5.0);
  (void)resource_add(state, RES_MYTHRIL, 3.0);
  notifs_add(state, "Trade complete.", 3.0f, PAL_GREEN_OK);
}

static bool can_bronze_10(const GameState *state) {
  return state && resource_has(state, RES_BRONZE, 10.0);
}

static void effect_merchant_trade_bronze(GameState *state) {
  (void)resource_sub(state, RES_BRONZE, 10.0);
  (void)resource_add(state, RES_COINS, 500.0);
  notifs_add(state, "Coins clink into your pouch.", 3.0f, PAL_GOLD);
}

static void effect_pulse_harness(GameState *state) {
  if (state->smelt_boost_timer < 30.0f) {
    state->smelt_boost_timer = 30.0f;
  }
  notifs_add(state, "Smelting speed surged!", 3.0f, PAL_RED);
}

static void effect_scrap_salvage(GameState *state) {
  (void)resource_add(state, RES_IRON_BAR, 5.0);
  (void)resource_add(state, RES_COPPER_BAR, 3.0);
  notifs_add(state, "You salvage usable metal.", 3.0f, PAL_GREEN_OK);
}

static void effect_scrap_sell(GameState *state) {
  (void)resource_add(state, RES_COINS, 350.0);
  notifs_add(state, "The market pays in full.", 3.0f, PAL_GOLD);
}

static void effect_cache_gears(GameState *state) {
  (void)resource_add(state, RES_GEAR, 12.0);
  notifs_add(state, "Gears clatter into your pack.", 3.0f, PAL_GREEN_OK);
}

static void effect_cache_plates(GameState *state) {
  (void)resource_add(state, RES_STEEL_PLATE, 4.0);
  notifs_add(state, "Heavy plates, solid workmanship.", 3.0f, PAL_GREEN_OK);
}

static void effect_cache_rivets(GameState *state) {
  (void)resource_add(state, RES_RIVET, 60.0);
  notifs_add(state, "A tin of rivets. Useful.", 3.0f, PAL_GREEN_OK);
}

static const EventChoice cave_in_choices[] = {
    {"Shore it up (cost: 50 stone)", can_stone_50, effect_cave_in_shore},
    {"Abandon it (-10% iron for 60s)", NULL, effect_cave_in_abandon},
};

static const EventChoice merchant_choices[] = {
    {"Trade 5 steel → 3 mythril", can_steel_5, effect_merchant_trade_steel},
    {"Trade 10 bronze → 500 coins", can_bronze_10, effect_merchant_trade_bronze},
    {"Send him away", NULL, NULL},
};

static const EventChoice pulse_choices[] = {
    {"Harness the pulse (2× smelt speed, 30s)", NULL, effect_pulse_harness},
    {"Let it pass", NULL, NULL},
};

static const EventChoice scrap_choices[] = {
    {"Salvage bars (+5 iron bars, +3 copper bars)", NULL, effect_scrap_salvage},
    {"Sell the scrap (+350 coins)", NULL, effect_scrap_sell},
    {"Leave it", NULL, NULL},
};

static const EventChoice cache_choices[] = {
    {"Take the gears (+12 gear)", NULL, effect_cache_gears},
    {"Take the plates (+4 steel plates)", NULL, effect_cache_plates},
    {"Take the rivets (+60 rivets)", NULL, effect_cache_rivets},
};

static const EventDef events[EVENT_COUNT] = {
    [EVENT_CAVE_IN] = {EVENT_CAVE_IN, "Cave-In!", "A tremor rocks the mine.\nDust fills the air and a tunnel collapses.", cave_in_choices, EF_ARRAY_COUNT(cave_in_choices)},
    [EVENT_WANDERING_MERCHANT] = {EVENT_WANDERING_MERCHANT,
                                  "Wandering Merchant",
                                  "A hooded trader offers rare goods.\nHe prefers bars and alloyed wares.",
                                  merchant_choices,
                                  EF_ARRAY_COUNT(merchant_choices)},
    [EVENT_EMBER_PULSE] = {EVENT_EMBER_PULSE,
                           "Ember Pulse",
                           "An ember crystal resonates.\nThe forge glows hotter for a moment.",
                           pulse_choices,
                           EF_ARRAY_COUNT(pulse_choices)},
    [EVENT_SCRAP_WINDFALL] = {EVENT_SCRAP_WINDFALL,
                              "Scrap Windfall",
                              "A broken caravan leaves behind a pile of scrap.\nNot all of it is useless.",
                              scrap_choices,
                              EF_ARRAY_COUNT(scrap_choices)},
    [EVENT_ANCIENT_CACHE] = {EVENT_ANCIENT_CACHE,
                             "Ancient Cache",
                             "You crack open a sealed coffer.\nInside: neatly packed components.",
                             cache_choices,
                             EF_ARRAY_COUNT(cache_choices)},
};

const EventDef *event_def(EventId id) {
  if (id <= EVENT_NONE || id >= EVENT_COUNT) {
    return NULL;
  }
  return &events[id];
}

bool start_random_event(GameState *state) {
  if (!state || state->active_event != EVENT_NONE) {
    return false;
  }
  int idx = rand() % EVENT_COUNT;
  const EventDef *ev = &events[idx];
  state->active_event = ev->id;
  notify_event(state, ev->name);
  return true;
}

bool resolve_active_event(GameState *state) {
  if (!state || state->active_event == EVENT_NONE) {
    return false;
  }
  state->active_event = EVENT_NONE;
  schedule_next_event(state);
  return true;
}

bool choose_active_event(GameState *state, size_t choice_index) {
  if (!state || state->active_event == EVENT_NONE) {
    return false;
  }
  const EventDef *ev = event_def(state->active_event);
  if (!ev || choice_index >= ev->choice_count) {
    return false;
  }
  const EventChoice *choice = &ev->choices[choice_index];
  if (choice->can && !choice->can(state)) {
    notifs_add(state, "You can't do that right now.", 3.0f, PAL_WARN);
    return false;
  }
  if (choice->effect) {
    choice->effect(state);
  }
  (void)resolve_active_event(state);
  return true;
}

bool smelt_boost_active(const GameState *state) {
  return state && state->smelt_boost_timer > 0.0f;
}

bool iron_penalty_active(const GameState *state) {
  return state && state->iron_penalty_timer > 0.0f;
}

void tick_events(GameState *state, float dt_seconds) {
  if (!state) {
    return;
  }

  if (smelt_boost_active(state)) {
    state->smelt_boost_timer -= dt_seconds;
    if (state->smelt_boost_timer <= 0.0f) {
      state->smelt_boost_timer = 0.0f;
      notifs_add(state, "Ember Pulse faded.", 3.0f, PAL_TEXT_DIM);
    }
  }

  if (iron_penalty_active(state)) {
    state->iron_penalty_timer -= dt_seconds;
    if (state->iron_penalty_timer <= 0.0f) {
      state->iron_penalty_timer = 0.0f;
      notifs_add(state, "The mine tunnels reopen.", 3.0f, PAL_TEXT_DIM);
    }
  }

  if (state->active_event == EVENT_NONE) {
    if (state->event_timer <= 0.0f) {
      state->event_timer = 90.0f;
    }
    state->event_timer -= dt_seconds;
    if (state->event_timer <= 0.0f) {
      (void)start_random_event(state);
    }
  }
}

typedef bool (*AchievementUnlockFn)(const GameState *state);

typedef struct AchievementDef {
  AchievementId id;
  const char *name;
  const char *description;
  AchievementUnlockFn unlock;
} AchievementDef;

static bool unlock_first_spark(const GameState *state) {
  return state && (resource_ever(state, RES_IRON_BAR) >= 1.0 || resource_ever(state, RES_COPPER_BAR) >= 1.0);
}

static bool unlock_iron_will(const GameState *state) {
  return state && building_count(state, BLD_IRON_MINE) >= 10;
}

static bool unlock_the_alchemist(const GameState *state) {
  return state && resource_ever(state, RES_BRONZE) >= 1.0;
}

static bool unlock_deep_echo(const GameState *state) {
  return state && resource_ever(state, RES_MYTHRIL) >= 1.0;
}

static bool unlock_ember_awakened(const GameState *state) {
  return state && resource_ever(state, RES_EMBER) >= 1.0;
}

static bool unlock_eternal_flame(const GameState *state) {
  return state && state->ascensions >= 5;
}

static bool unlock_stone_stockpile(const GameState *state) {
  return state && resource_ever(state, RES_STONE) >= 1000.0;
}

static bool unlock_coal_baron(const GameState *state) {
  return state && resource_ever(state, RES_COAL) >= 500.0;
}

static bool unlock_pickaxe_army(const GameState *state) {
  return state && building_count(state, BLD_PICKAXE) >= 25;
}

static bool unlock_furnace_fleet(const GameState *state) {
  return state && building_count(state, BLD_FURNACE) >= 3;
}

static bool unlock_bellows_business(const GameState *state) {
  return state && building_count(state, BLD_BELLOWS_SHOP) >= 2;
}

static bool unlock_merchant_prince(const GameState *state) {
  return state && resource_ever(state, RES_COINS) >= 10000.0;
}

static bool unlock_gearhead(const GameState *state) {
  return state && resource_ever(state, RES_GEAR) >= 50.0;
}

static bool unlock_airflow_artisan(const GameState *state) {
  return state && resource_ever(state, RES_BELLOWS) >= 10.0;
}

static bool unlock_bronze_age(const GameState *state) {
  return state && resource_ever(state, RES_BRONZE) >= 25.0;
}

static bool unlock_steelworks(const GameState *state) {
  return state && resource_ever(state, RES_STEEL) >= 25.0;
}

static bool unlock_plate_press(const GameState *state) {
  return state && resource_ever(state, RES_STEEL_PLATE) >= 10.0;
}

static bool unlock_rivet_rain(const GameState *state) {
  return state && resource_ever(state, RES_RIVET) >= 200.0;
}

static bool unlock_parts_bin(const GameState *state) {
  return state && resource_ever(state, RES_MACHINE_PART) >= 3.0;
}

static bool unlock_shopaholic(const GameState *state) {
  if (!state) {
    return false;
  }
  int sum = 0;
  for (int i = 0; i < SHOP_COUNT; ++i) {
    sum += state->shop_purchases[i];
  }
  return sum >= 5;
}

static bool unlock_queue_master(const GameState *state) {
  return state && state->smelt_queue.len >= 8;
}

static bool unlock_tinkerer(const GameState *state) {
  if (!state) {
    return false;
  }
  int count = 0;
  for (int i = 0; i < UPG_COUNT; ++i) {
    if (state->upgrades[i]) {
      count += 1;
    }
  }
  return count >= 10;
}

static bool unlock_industrial_revolution(const GameState *state) {
  return state && state->upgrades[UPG_ASSEMBLY_LINES];
}

static const AchievementDef achievements[ACH_COUNT] = {
    [ACH_FIRST_SPARK] = {ACH_FIRST_SPARK, "First Spark", "Smelt your first bar.", unlock_first_spark},
    [ACH_IRON_WILL] = {ACH_IRON_WILL, "Iron Will", "Own 10 Iron Mines.", unlock_iron_will},
    [ACH_THE_ALCHEMIST] = {ACH_THE_ALCHEMIST, "The Alchemist", "Discover the Bronze recipe.", unlock_the_alchemist},
    [ACH_DEEP_ECHO] = {ACH_DEEP_ECHO, "Deep Echo", "Extract your first Mythril.", unlock_deep_echo},
    [ACH_EMBER_AWAKENED] = {ACH_EMBER_AWAKENED, "Ember Awakened", "Condense your first Ember Crystal.", unlock_ember_awakened},
    [ACH_ETERNAL_FLAME] = {ACH_ETERNAL_FLAME, "Eternal Flame", "Ascend 5 times.", unlock_eternal_flame},
    [ACH_STONE_STOCKPILE] = {ACH_STONE_STOCKPILE, "Stone Stockpile", "Produce 1,000 stone.", unlock_stone_stockpile},
    [ACH_COAL_BARON] = {ACH_COAL_BARON, "Coal Baron", "Produce 500 coal.", unlock_coal_baron},
    [ACH_PICKAXE_ARMY] = {ACH_PICKAXE_ARMY, "Pickaxe Army", "Own 25 Pickaxes.", unlock_pickaxe_army},
    [ACH_FURNACE_FLEET] = {ACH_FURNACE_FLEET, "Furnace Fleet", "Own 3 Auto-Furnaces.", unlock_furnace_fleet},
    [ACH_BELLOWS_BUSINESS] = {ACH_BELLOWS_BUSINESS, "Bellows Business", "Own 2 Bellows Workshops.", unlock_bellows_business},
    [ACH_MERCHANT_PRINCE] = {ACH_MERCHANT_PRINCE, "Merchant Prince", "Produce 10,000 coins.", unlock_merchant_prince},
    [ACH_GEARHEAD] = {ACH_GEARHEAD, "Gearhead", "Produce 50 gears.", unlock_gearhead},
    [ACH_AIRFLOW_ARTISAN] = {ACH_AIRFLOW_ARTISAN, "Airflow Artisan", "Craft 10 bellows.", unlock_airflow_artisan},
    [ACH_BRONZE_AGE] = {ACH_BRONZE_AGE, "Bronze Age", "Produce 25 bronze bars.", unlock_bronze_age},
    [ACH_STEELWORKS] = {ACH_STEELWORKS, "Steelworks", "Produce 25 steel bars.", unlock_steelworks},
    [ACH_PLATE_PRESS] = {ACH_PLATE_PRESS, "Plate Press", "Hammer 10 steel plates.", unlock_plate_press},
    [ACH_RIVET_RAIN] = {ACH_RIVET_RAIN, "Rivet Rain", "Forge 200 rivets.", unlock_rivet_rain},
    [ACH_PARTS_BIN] = {ACH_PARTS_BIN, "Parts Bin", "Assemble 3 machine parts.", unlock_parts_bin},
    [ACH_SHOPAHOLIC] = {ACH_SHOPAHOLIC, "Shopaholic", "Buy 5 shop perks total.", unlock_shopaholic},
    [ACH_QUEUE_MASTER] = {ACH_QUEUE_MASTER, "Queue Master", "Fill 8 smelt slots.", unlock_queue_master},
    [ACH_TINKERER] = {ACH_TINKERER, "Tinkerer", "Buy 10 upgrades.", unlock_tinkerer},
    [ACH_INDUSTRIAL_REVOLUTION] = {ACH_INDUSTRIAL_REVOLUTION, "Industrial Revolution", "Buy Assembly Lines.", unlock_industrial_revolution},
};

static const AchievementDef *achievement_def(AchievementId id) {
  if ((int)id < 0 || id >= ACH_COUNT) {
    return NULL;
  }
  return &achievements[id];
}

bool achievement_unlocked(const GameState *state, AchievementId id) {
  return state && state->achievements[id];
}

bool unlock_achievement(GameState *state, AchievementId id) {
  if (!state) {
    return false;
  }
  const AchievementDef *def = achievement_def(id);
  if (!def) {
    return false;
  }
  if (achievement_unlocked(state, id)) {
    return false;
  }
  state->achievements[id] = true;
  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Achievement: %s", def->name);
  notifs_add(state, msg, 3.0f, PAL_GOLD);
  return true;
}

void maybe_unlock_achievements(GameState *state) {
  if (!state) {
    return;
  }
  for (int i = 0; i < ACH_COUNT; ++i) {
    const AchievementDef *def = &achievements[i];
    if (!def->unlock) {
      continue;
    }
    if (!achievement_unlocked(state, def->id) && def->unlock(state)) {
      (void)unlock_achievement(state, def->id);
    }
  }
}

