#pragma once

#include "ef_defs.h"
#include "ef_palette.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 768 };

typedef struct CrucibleSlot {
  bool     has_recipe; /* is a recipe pinned? */
  RecipeId recipe;
  float    progress;   /* 0..1 while running */
  float    duration;   /* cached from RecipeDef */
  bool     stalled;    /* can't afford inputs right now */
} CrucibleSlot;

typedef struct BuildingInstance {
  int count;
  int level;
} BuildingInstance;

typedef struct SmeltJob {
  RecipeId recipe;
  float progress;
  float duration;
  SmeltSource source;
} SmeltJob;

typedef struct SmeltQueue {
  SmeltJob *items;
  size_t len;
  size_t cap;
} SmeltQueue;

typedef struct Notif {
  char text[256];
  float timer;
  PaletteColor color;
} Notif;

typedef struct NotifList {
  Notif items[16];
  size_t len;
} NotifList;

typedef struct ManualJob {
  bool active;
  ResourceId resource;
  double amount;
  float timer;
  float duration;
} ManualJob;

typedef struct GameState {
  uint64_t tick;
  uint64_t last_save_universal;
  float dt_seconds;

  double resources[RES_COUNT];
  double caps[RES_COUNT];
  double ever_produced[RES_COUNT];
  bool unlocked[RES_COUNT];
  double production[RES_COUNT];

  BuildingInstance buildings[BLD_COUNT];
  double building_mults[BLD_COUNT];

  bool upgrades[UPG_COUNT];

  double essence;
  int ascensions;
  double prod_mult;
  double click_mult;
  double gather_mult;
  double smelt_speed_mult;

  bool eternal_upgrades[ETERNAL_COUNT];

  double sell_mult;
  double shop_discount;
  int shop_purchases[SHOP_COUNT];

  float manual_mine_duration;
  float manual_gather_duration;
  ManualJob manual_mine;
  ManualJob manual_gather;

  float event_timer;
  EventId active_event;
  float smelt_boost_timer;
  float iron_penalty_timer;

  bool achievements[ACH_COUNT];

  int save_slot;
  bool autosave_enabled;
  int autosave_interval_seconds;
  bool offline_progress_enabled;
  bool fullscreen_enabled;
  char font_path[1024];
  char slot_names[3][32]; /* player-chosen name for each save slot */

  Screen screen;
  bool game_started;
  Panel active_panel;

  NotifList notifications;
  int smelt_slots_extra;

  int          crucibles_purchased;          /* 0..MAX_CRUCIBLES */
  CrucibleSlot crucibles[MAX_CRUCIBLES];

  /* Bitmask per resource: bit i set = milestone index i already fired.
   * Persisted in the save file so notifications never repeat across sessions. */
  uint8_t milestones_fired[RES_COUNT];

  int fps;
  int frame_counter;
  float frame_accum_seconds;

  SmeltQueue smelt_queue;
} GameState;

GameState *game_state_create(void);
void game_state_destroy(GameState *state);

void game_state_reset_transient(GameState *state);
void game_state_hard_reset(GameState *state);

float manual_job_progress(const ManualJob *job);

void notifs_add(GameState *state, const char *text, float timer_seconds, PaletteColor color);
void notifs_tick(GameState *state, float dt_seconds);

void smelt_queue_init(SmeltQueue *queue);
void smelt_queue_destroy(SmeltQueue *queue);
bool smelt_queue_insert_front(SmeltQueue *queue, SmeltJob job);
bool smelt_queue_remove_at(SmeltQueue *queue, size_t index);

