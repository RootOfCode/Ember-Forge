#pragma once

#include "ef_defs.h"
#include "ef_palette.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct GameState GameState;

typedef struct ResAmount {
  ResourceId res;
  double amount;
} ResAmount;

typedef struct ResourceDef {
  const char  *name;
  PaletteColor color;
  int          tier;
  double       base_cap;
  double       sell_value;
  float        base_gather_time; /* seconds for one manual gather; 0 = not gatherable */
} ResourceDef;

const ResourceDef *resource_def(ResourceId resource);
const char        *resource_name(ResourceId resource);
PaletteColor       resource_color(ResourceId resource);
int                resource_tier(ResourceId resource);
double             resource_base_cap(ResourceId resource);
double             sell_value(ResourceId resource);
/* Base gather time in seconds (before gather_mult scaling). 0 = not gatherable. */
float              resource_base_gather_time(ResourceId resource);

void   init_resource_caps(GameState *state);
double resource_cap(GameState *state, ResourceId resource);
double resource_amount(const GameState *state, ResourceId resource);
double resource_ever(const GameState *state, ResourceId resource);

bool   resource_unlocked_p(const GameState *state, ResourceId resource);
void   resource_unlock(GameState *state, ResourceId resource);
bool   resource_has(const GameState *state, ResourceId resource, double amount);
double resource_add(GameState *state, ResourceId resource, double delta);
double resource_sub(GameState *state, ResourceId resource, double delta);

void maybe_unlock_produced_resources(GameState *state);

bool can_afford_list(const GameState *state, const ResAmount *items, size_t item_count);
void spend_list(GameState *state, const ResAmount *items, size_t item_count);

bool can_sell(const GameState *state, ResourceId resource, double qty);
bool sell_resource(GameState *state, ResourceId resource, double qty);
