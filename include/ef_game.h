#pragma once

#include "ef_defs.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GameState GameState;

bool start_manual_mine(GameState *state);
bool start_manual_gather(GameState *state, ResourceId resource);
void tick_manual_jobs(GameState *state, float dt_seconds);

void update_fps(GameState *state, float dt_seconds);
void tick_game(GameState *state, float dt_seconds);

void apply_offline_production(GameState *state);

bool start_continue_game(GameState *state);
bool start_new_game(GameState *state);

void maybe_autosave(GameState *state);

