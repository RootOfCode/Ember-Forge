#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct GameState GameState;

bool save_game(GameState *state);
bool load_save(GameState *state);
bool save_exists(const GameState *state, int slot);
bool delete_save(GameState *state, int slot);

/* Returns the unix timestamp stored in a slot's save file, or 0 if absent. */
uint64_t save_slot_saved_at(int slot);

bool save_options(const GameState *state);
bool load_options(GameState *state);

