#include "ef_state.h"

#include "ef_buildings.h"
#include "ef_recipes.h"
#include "ef_upgrades.h"

#include <stdlib.h>
#include <string.h>

void smelt_queue_init(SmeltQueue *queue) {
  if (!queue) {
    return;
  }
  queue->items = NULL;
  queue->len = 0;
  queue->cap = 0;
}

void smelt_queue_destroy(SmeltQueue *queue) {
  if (!queue) {
    return;
  }
  free(queue->items);
  queue->items = NULL;
  queue->len = 0;
  queue->cap = 0;
}

static bool smelt_queue_reserve(SmeltQueue *queue, size_t cap) {
  if (!queue) {
    return false;
  }
  if (queue->cap >= cap) {
    return true;
  }
  size_t new_cap = queue->cap > 0 ? queue->cap : 16;
  while (new_cap < cap) {
    new_cap *= 2;
  }
  void *new_items = realloc(queue->items, new_cap * sizeof(SmeltJob));
  if (!new_items) {
    return false;
  }
  queue->items = (SmeltJob *)new_items;
  queue->cap = new_cap;
  return true;
}

bool smelt_queue_insert_front(SmeltQueue *queue, SmeltJob job) {
  if (!queue) {
    return false;
  }
  if (!smelt_queue_reserve(queue, queue->len + 1)) {
    return false;
  }
  if (queue->len > 0) {
    memmove(&queue->items[1], &queue->items[0], queue->len * sizeof(SmeltJob));
  }
  queue->items[0] = job;
  queue->len += 1;
  return true;
}

bool smelt_queue_remove_at(SmeltQueue *queue, size_t index) {
  if (!queue || index >= queue->len) {
    return false;
  }
  if (index + 1 < queue->len) {
    memmove(&queue->items[index], &queue->items[index + 1], (queue->len - index - 1) * sizeof(SmeltJob));
  }
  queue->len -= 1;
  return true;
}

static void notifs_clear(NotifList *list) {
  if (!list) {
    return;
  }
  memset(list, 0, sizeof(*list));
}

void notifs_add(GameState *state, const char *text, float timer_seconds, PaletteColor color) {
  if (!state || !text || text[0] == '\0') {
    return;
  }

  NotifList *list = &state->notifications;
  if (list->len >= (sizeof(list->items) / sizeof(list->items[0]))) {
    list->len = (sizeof(list->items) / sizeof(list->items[0])) - 1;
  }

  if (list->len > 0) {
    memmove(&list->items[1], &list->items[0], list->len * sizeof(list->items[0]));
  }

  Notif *n = &list->items[0];
  memset(n, 0, sizeof(*n));
  strncpy(n->text, text, sizeof(n->text) - 1);
  n->text[sizeof(n->text) - 1] = '\0';
  n->timer = timer_seconds > 0.0f ? timer_seconds : 0.0f;
  n->color = color;
  list->len += 1;
}

void notifs_tick(GameState *state, float dt_seconds) {
  if (!state) {
    return;
  }
  NotifList *list = &state->notifications;
  size_t out_len = 0;
  for (size_t i = 0; i < list->len; ++i) {
    Notif n = list->items[i];
    n.timer -= dt_seconds;
    if (n.timer > 0.0f) {
      list->items[out_len++] = n;
    }
  }
  list->len = out_len;
}

/* ── Milestone popup queue ───────────────────────────────────────────────── */

void milestone_popups_add(GameState *state, const char *text, float duration,
                          PaletteColor color) {
  if (!state || !text || text[0] == '\0') return;
  MilestoneQueue *q = &state->milestone_popups;
  /* If full, drop the oldest entry to make room */
  if (q->len >= MILESTONE_QUEUE_MAX)
    q->len = MILESTONE_QUEUE_MAX - 1;
  /* Shift existing entries down one slot (newest at index 0) */
  if (q->len > 0)
    memmove(&q->items[1], &q->items[0], q->len * sizeof(q->items[0]));
  MilestonePopup *p = &q->items[0];
  memset(p, 0, sizeof(*p));
  strncpy(p->text, text, sizeof(p->text) - 1);
  p->timer    = duration > 0.0f ? duration : 6.0f;
  p->duration = p->timer;
  p->color    = color;
  q->len     += 1;
}

void milestone_popups_tick(GameState *state, float dt_seconds) {
  if (!state) return;
  MilestoneQueue *q = &state->milestone_popups;
  size_t out = 0;
  for (size_t i = 0; i < q->len; ++i) {
    q->items[i].timer -= dt_seconds;
    if (q->items[i].timer > 0.0f)
      q->items[out++] = q->items[i];
  }
  q->len = out;
}

float manual_job_progress(const ManualJob *job) {
  if (!job || !job->active) {
    return 0.0f;
  }
  if (job->duration <= 0.0f) {
    return 0.0f;
  }
  float p = 1.0f - (job->timer / job->duration);
  if (p < 0.0f) {
    p = 0.0f;
  }
  if (p > 1.0f) {
    p = 1.0f;
  }
  return p;
}

static void state_defaults(GameState *state) {
  if (!state) {
    return;
  }
  memset(state, 0, sizeof(*state));

  state->save_slot = 1;
  state->autosave_enabled = true;
  state->autosave_interval_seconds = 60;
  state->offline_progress_enabled = true;
  state->fullscreen_enabled = false;
  strncpy(state->font_path,
          "assets/fonts/JetBrainsMono/fonts/ttf/JetBrainsMono-Regular.ttf",
          sizeof(state->font_path) - 1);

  state->screen = SCREEN_MENU;
  state->game_started = false;
  state->active_panel = PANEL_FORGE;

  state->active_event = EVENT_NONE;

  state->smelt_queue.items = NULL;
  state->smelt_queue.len = 0;
  state->smelt_queue.cap = 0;
  smelt_queue_init(&state->smelt_queue);

  notifs_clear(&state->notifications);
}

void game_state_reset_transient(GameState *state) {
  if (!state) {
    return;
  }
  state->manual_mine.active = false;
  state->manual_gather.active = false;
  state->active_event = EVENT_NONE;
  notifs_clear(&state->notifications);
  memset(&state->milestone_popups, 0, sizeof(state->milestone_popups));
}

void game_state_hard_reset(GameState *state) {
  if (!state) {
    return;
  }

  state->tick = 0;
  state->last_save_universal = 0;
  state->dt_seconds = 0.0f;

  memset(state->resources, 0, sizeof(state->resources));
  memset(state->caps, 0, sizeof(state->caps));
  memset(state->ever_produced, 0, sizeof(state->ever_produced));
  memset(state->production, 0, sizeof(state->production));

  for (int i = 0; i < RES_COUNT; ++i) {
    state->unlocked[i] = false;
  }
  state->unlocked[RES_STONE] = true;
  state->unlocked[RES_COAL] = true;
  state->unlocked[RES_COINS] = true;

  for (int i = 0; i < BLD_COUNT; ++i) {
    state->buildings[i] = (BuildingInstance){0, 1};
    state->building_mults[i] = 1.0;
  }

  memset(state->upgrades, 0, sizeof(state->upgrades));

  state->essence = 0.0;
  state->ascensions = 0;
  state->prod_mult = 1.0;
  state->click_mult = 1.0;
  state->gather_mult = 1.0;
  state->smelt_speed_mult = 1.0;

  memset(state->eternal_upgrades, 0, sizeof(state->eternal_upgrades));
  memset(state->achievements, 0, sizeof(state->achievements));
  memset(state->milestones_fired, 0, sizeof(state->milestones_fired));

  state->sell_mult = 1.0;
  state->shop_discount = 1.0;
  memset(state->shop_purchases, 0, sizeof(state->shop_purchases));

  state->manual_mine_duration = 5.0f;
  state->manual_gather_duration = 6.0f;
  state->manual_mine = (ManualJob){0};
  state->manual_gather = (ManualJob){0};

  state->event_timer = 90.0f;
  state->active_event = EVENT_NONE;
  state->smelt_boost_timer = 0.0f;
  state->iron_penalty_timer = 0.0f;

  state->active_panel = PANEL_FORGE;
  notifs_clear(&state->notifications);
  memset(&state->milestone_popups, 0, sizeof(state->milestone_popups));
  state->smelt_slots_extra = 0;
  state->crucibles_purchased = 0;
  memset(state->crucibles, 0, sizeof(state->crucibles));

  state->fps = 0;
  state->frame_counter = 0;
  state->frame_accum_seconds = 0.0f;

  state->smelt_queue.len = 0;

  state->resources[RES_COINS] = 25.0;
  state->ever_produced[RES_COINS] = 25.0;
}

GameState *game_state_create(void) {
  // Populate all definition tables from the DSL macros.
  buildings_init();
  upgrades_init();
  recipes_init();
  GameState *state = (GameState *)malloc(sizeof(GameState));
  if (!state) {
    return NULL;
  }
  state_defaults(state);
  game_state_hard_reset(state);
  return state;
}

void game_state_destroy(GameState *state) {
  if (!state) {
    return;
  }
  smelt_queue_destroy(&state->smelt_queue);
  free(state);
}

