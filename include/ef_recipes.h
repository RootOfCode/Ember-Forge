#pragma once

#include "ef_defs.h"
#include "ef_resources.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct GameState GameState;

typedef bool (*RecipeUnlockFn)(const GameState *state);

typedef struct RecipeDef {
  RecipeId id;
  const char *name;
  const ResAmount *inputs;
  size_t input_count;
  const ResAmount *outputs;
  size_t output_count;
  float duration_seconds;
  RecipeUnlockFn unlock;
} RecipeDef;

void recipes_init(void);

const RecipeDef *recipe_def(RecipeId id);
const char *recipe_name(RecipeId id);

bool recipe_available(const GameState *state, const RecipeDef *def);
bool can_afford_recipe(const GameState *state, const RecipeDef *def);

bool enqueue_smelt_job(GameState *state, RecipeId recipe_id, SmeltSource source);
bool cancel_smelt_job(GameState *state, size_t index);

void tick_smelt_queue(GameState *state, float dt_seconds);
void tick_auto_furnaces(GameState *state);

/* Dedicated crucibles — each slot auto-repeats one pinned recipe. */
void crucible_set_recipe(GameState *state, int slot, RecipeId recipe);
void crucible_start(GameState *state, int slot);
void crucible_stop(GameState *state, int slot);
void tick_crucibles(GameState *state, float dt_seconds);

