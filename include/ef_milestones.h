#pragma once

/*
 * ef_milestones.h — Resource milestone notifications
 *
 * Milestones fire a special notification the first time a resource's
 * cumulative ever_produced total crosses a round-number threshold.
 * Each milestone fires exactly once per save (state->milestones_fired
 * persists across sessions).
 *
 * Thresholds are tuned per resource tier so early-game resources start
 * at larger round numbers and late-game rarities celebrate the first one.
 */

#include "ef_defs.h"

typedef struct GameState GameState;

/*
 * Check whether any new milestones have been crossed for resource `r`
 * and fire notifications for each one.  Call this after any increase to
 * state->ever_produced[r].
 */
void milestones_check(GameState *state, ResourceId r);
