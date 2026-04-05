#pragma once

#include "ef_defs.h"
#include "ef_resources.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct GameState GameState;

typedef bool (*BuildingUnlockFn)(const GameState *state);

typedef struct BuildingDef {
  BuildingId id;
  const char *name;
  const char *description;
  const ResAmount *base_cost;
  size_t base_cost_count;
  double cost_scale;
  const ResAmount *production;
  size_t production_count;
  BuildingUnlockFn unlock;
} BuildingDef;

void buildings_init(void);

const BuildingDef *building_def(BuildingId id);
const char *building_name(BuildingId id);
const char *building_description(BuildingId id);

int building_count(const GameState *state, BuildingId id);
double building_prod_mult(const GameState *state, BuildingId id);

void multiply_building_prod(GameState *state, BuildingId id, double factor);

bool building_unlocked(const GameState *state, const BuildingDef *def);

size_t building_cost(BuildingId id, int owned, ResAmount *out_items, size_t out_cap);
bool buy_building(GameState *state, BuildingId id);

void recompute_production(GameState *state);
void tick_buildings(GameState *state, float dt_seconds);

int max_smelt_slots(const GameState *state);
double smelt_speed_multiplier(const GameState *state);

