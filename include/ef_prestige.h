#pragma once

#include "ef_defs.h"

#include <stdbool.h>

typedef struct GameState GameState;

bool eternal_upgrade_owned(const GameState *state, EternalUpgradeId id);
bool buy_eternal_upgrade(GameState *state, EternalUpgradeId id);

void apply_eternal_start_bonuses(GameState *state);

bool prestige_available(const GameState *state);
int essence_gain(const GameState *state);

void recompute_prestige_mults(GameState *state);
bool ascend(GameState *state);

