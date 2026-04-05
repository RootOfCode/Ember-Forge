#pragma once

#include "ef_defs.h"
#include "ef_resources.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct GameState GameState;

typedef void (*UpgradeEffectFn)(GameState *state);
typedef bool (*UpgradeUnlockFn)(const GameState *state);

typedef struct UpgradeDef {
  UpgradeId id;
  const char *name;
  const char *description;
  const ResAmount *cost;
  size_t cost_count;
  UpgradeEffectFn effect;
  UpgradeUnlockFn unlock;
} UpgradeDef;

void upgrades_init(void);

const UpgradeDef *upgrade_def(UpgradeId id);
const char *upgrade_name(UpgradeId id);
const char *upgrade_description(UpgradeId id);

bool upgrade_owned(const GameState *state, UpgradeId id);
bool upgrade_available(const GameState *state, const UpgradeDef *def);

bool buy_upgrade(GameState *state, UpgradeId id);

