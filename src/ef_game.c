#include "ef_game.h"

#include "ef_buildings.h"
#include "ef_events.h"
#include "ef_format.h"
#include "ef_prestige.h"
#include "ef_recipes.h"
#include "ef_resources.h"
#include "ef_save.h"
#include "ef_state.h"
#include "ef_time.h"

#include <stdio.h>

bool start_manual_mine(GameState *state) {
  if (!state || state->manual_mine.active) {
    return false;
  }
  float dur = state->manual_mine_duration;
  if (dur < 0.1f) {
    dur = 0.1f;
  }
  state->manual_mine.active = true;
  state->manual_mine.resource = RES_STONE;
  state->manual_mine.amount = state->click_mult;
  state->manual_mine.timer = dur;
  state->manual_mine.duration = dur;
  return true;
}

bool start_manual_gather(GameState *state, ResourceId resource) {
  if (!state || state->manual_gather.active) {
    return false;
  }
  /* Use the per-resource base gather time from the DSL, scaled by the
   * player's current gather_mult (higher mult = faster gather).
   * Falls back to manual_gather_duration for resources with no defined time. */
  float base = resource_base_gather_time(resource);
  float dur;
  if (base > 0.0f) {
    /* gather_mult > 1 → divide time; guard against division by zero / negatives */
    double mult = state->gather_mult > 0.1 ? state->gather_mult : 0.1;
    dur = base / (float)mult;
  } else {
    dur = state->manual_gather_duration;
  }
  if (dur < 0.2f) {
    dur = 0.2f;
  }
  state->manual_gather.active   = true;
  state->manual_gather.resource = resource;
  state->manual_gather.amount   = state->gather_mult;
  state->manual_gather.timer    = dur;
  state->manual_gather.duration = dur;
  return true;
}

static void notify_gain(GameState *state, double amount, ResourceId resource) {
  char qty_buf[64];
  fmt_number(qty_buf, sizeof(qty_buf), amount);
  char msg[256];
  (void)snprintf(msg, sizeof(msg), "+%s %s", qty_buf, resource_name(resource));
  notifs_add(state, msg, 3.0f, PAL_GREEN_OK);
}

void tick_manual_jobs(GameState *state, float dt_seconds) {
  if (!state) {
    return;
  }

  if (state->manual_mine.active) {
    state->manual_mine.timer -= dt_seconds;
    if (state->manual_mine.timer <= 0.0f) {
      (void)resource_add(state, state->manual_mine.resource, state->manual_mine.amount);
      notify_gain(state, state->manual_mine.amount, state->manual_mine.resource);
      state->manual_mine.active = false;
    }
  }

  if (state->manual_gather.active) {
    state->manual_gather.timer -= dt_seconds;
    if (state->manual_gather.timer <= 0.0f) {
      (void)resource_add(state, state->manual_gather.resource, state->manual_gather.amount);
      notify_gain(state, state->manual_gather.amount, state->manual_gather.resource);
      state->manual_gather.active = false;
    }
  }
}

void update_fps(GameState *state, float dt_seconds) {
  if (!state) {
    return;
  }
  state->frame_counter += 1;
  state->frame_accum_seconds += dt_seconds;
  if (state->frame_accum_seconds >= 1.0f) {
    state->fps = state->frame_counter;
    state->frame_counter = 0;
    state->frame_accum_seconds -= 1.0f;
  }
}

void tick_game(GameState *state, float dt_seconds) {
  if (!state) {
    return;
  }
  state->tick += 1;

  tick_manual_jobs(state, dt_seconds);
  tick_buildings(state, dt_seconds);
  tick_auto_furnaces(state);
  tick_smelt_queue(state, dt_seconds);
  tick_crucibles(state, dt_seconds);
  tick_events(state, dt_seconds);
  notifs_tick(state, dt_seconds);

  if ((state->tick % 30u) == 0u) {
    maybe_unlock_produced_resources(state);
  }
  if ((state->tick % 60u) == 0u) {
    maybe_unlock_achievements(state);
  }

  update_fps(state, dt_seconds);
}

void apply_offline_production(GameState *state) {
  if (!state) {
    return;
  }

  uint64_t last = state->last_save_universal;
  uint64_t now = universal_time_now();
  if (last == 0 || now <= last) {
    return;
  }

  uint64_t elapsed = now - last;
  if (elapsed > 28800u) {
    elapsed = 28800u;
  }
  if (elapsed <= 1u) {
    return;
  }

  recompute_production(state);
  double sec = (double)elapsed;
  for (int i = 0; i < RES_COUNT; ++i) {
    double rate = state->production[i];
    if (rate != 0.0) {
      (void)resource_add(state, (ResourceId)i, rate * sec);
    }
  }

  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Offline: %llu seconds", (unsigned long long)elapsed);
  notifs_add(state, msg, 3.0f, PAL_TEXT_DIM);
}

bool start_continue_game(GameState *state) {
  if (!state) {
    return false;
  }
  init_resource_caps(state);
  recompute_production(state);
  (void)load_save(state);
  game_state_reset_transient(state);
  apply_eternal_start_bonuses(state);
  recompute_production(state);
  maybe_unlock_produced_resources(state);
  if (state->offline_progress_enabled) {
    apply_offline_production(state);
  }
  state->screen = SCREEN_GAME;
  state->game_started = true;
  return true;
}

bool start_new_game(GameState *state) {
  if (!state) {
    return false;
  }
  game_state_hard_reset(state);
  init_resource_caps(state);
  recompute_production(state);
  state->screen = SCREEN_GAME;
  state->game_started = true;
  return true;
}

void maybe_autosave(GameState *state) {
  if (!state || !state->autosave_enabled) {
    return;
  }
  uint64_t now = universal_time_now();
  int interval = state->autosave_interval_seconds;
  if (interval < 10) {
    interval = 10;
  }
  if (state->last_save_universal == 0 || now - state->last_save_universal >= (uint64_t)interval) {
    if (save_game(state)) {
      state->last_save_universal = now;
    }
  }
}

