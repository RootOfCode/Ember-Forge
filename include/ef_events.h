#pragma once

#include "ef_defs.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct GameState GameState;

typedef bool (*EventChoiceCanFn)(const GameState *state);
typedef void (*EventChoiceEffectFn)(GameState *state);

typedef struct EventChoice {
  const char *label;
  EventChoiceCanFn can;
  EventChoiceEffectFn effect;
} EventChoice;

typedef struct EventDef {
  EventId id;
  const char *name;
  const char *text;
  const EventChoice *choices;
  size_t choice_count;
} EventDef;

const EventDef *event_def(EventId id);

void schedule_next_event(GameState *state);
bool start_random_event(GameState *state);
bool choose_active_event(GameState *state, size_t choice_index);
bool resolve_active_event(GameState *state);

bool smelt_boost_active(const GameState *state);
bool iron_penalty_active(const GameState *state);
void tick_events(GameState *state, float dt_seconds);

bool achievement_unlocked(const GameState *state, AchievementId id);
bool unlock_achievement(GameState *state, AchievementId id);
void maybe_unlock_achievements(GameState *state);

