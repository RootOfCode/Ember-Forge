#include "ef_shop.h"

#include "ef_buildings.h"
#include "ef_format.h"
#include "ef_math.h"
#include "ef_resources.h"
#include "ef_state.h"
#include "ef_util.h"

#include <math.h>
#include <stdio.h>

double shop_buy_mult(const GameState *state) {
  if (!state) {
    return 2.5;
  }
  double sell = state->sell_mult;
  double disc = state->shop_discount;
  if (disc < 0.01) {
    disc = 0.01;
  }
  double min_mult = (sell * 1.05) / disc;
  return min_mult > 2.5 ? min_mult : 2.5;
}

double shop_buy_cost(const GameState *state, ResourceId resource, double qty) {
  double q = qty > 0.0 ? qty : 0.0;
  double base = sell_value(resource);
  double disc = state ? state->shop_discount : 1.0;
  return base * q * shop_buy_mult(state) * disc;
}

bool shop_can_buy_resource(const GameState *state, ResourceId resource, double qty) {
  if (!state) {
    return false;
  }
  if (resource == RES_COINS || sell_value(resource) <= 0.0) {
    return false;
  }

  double q = qty > 0.0 ? qty : 0.0;
  double amt = resource_amount(state, resource);
  double cap = resource_cap((GameState *)state, resource);
  double room = cap - amt;
  if (q <= 0.0 || room < q) {
    return false;
  }
  return resource_has(state, RES_COINS, shop_buy_cost(state, resource, q));
}

bool buy_resource(GameState *state, ResourceId resource, double qty) {
  if (!state) {
    return false;
  }
  if (resource == RES_COINS || sell_value(resource) <= 0.0) {
    return false;
  }

  double q = qty > 0.0 ? qty : 0.0;
  double amt = resource_amount(state, resource);
  double cap = resource_cap(state, resource);
  double room = cap - amt;
  if (room < 0.0) {
    room = 0.0;
  }

  double buy_q = q < room ? q : room;
  double cost = shop_buy_cost(state, resource, buy_q);

  if (buy_q <= 0.0 || !resource_has(state, RES_COINS, cost)) {
    return false;
  }

  (void)resource_sub(state, RES_COINS, cost);
  (void)resource_add(state, resource, buy_q);

  char qty_buf[64];
  fmt_number(qty_buf, sizeof(qty_buf), buy_q);
  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Bought %s %s", qty_buf, resource_name(resource));
  notifs_add(state, msg, 3.0f, PAL_GOLD);
  return true;
}

int shop_item_level(const GameState *state, ShopItemId id) {
  if (!state || (int)id < 0 || id >= SHOP_COUNT) {
    return 0;
  }
  return state->shop_purchases[id];
}

bool shop_item_owned(const GameState *state, ShopItemId id) {
  return shop_item_level(state, id) > 0;
}

typedef bool (*ShopUnlockFn)(const GameState *state);
typedef void (*ShopApplyFn)(GameState *state, int new_level);

typedef struct ShopItemDef {
  ShopItemId id;
  const char *name;
  const char *description;
  double base_cost;
  double cost_scale;
  int max_level; // 0 = unlimited
  ShopUnlockFn unlock;
  ShopApplyFn apply;
} ShopItemDef;

static bool unlock_ever_coins_120(const GameState *state) {
  return state && resource_ever(state, RES_COINS) >= 120.0;
}

static bool unlock_ever_coins_150(const GameState *state) {
  return state && resource_ever(state, RES_COINS) >= 150.0;
}

static bool unlock_ever_stone_200(const GameState *state) {
  return state && resource_ever(state, RES_STONE) >= 200.0;
}

static bool unlock_ever_iron_bar_5(const GameState *state) {
  return state && resource_ever(state, RES_IRON_BAR) >= 5.0;
}

static bool unlock_ever_bronze_5(const GameState *state) {
  return state && resource_ever(state, RES_BRONZE) >= 5.0;
}

static void multiply_all_caps(GameState *state, double factor) {
  if (!state) {
    return;
  }
  for (int i = 0; i < RES_COUNT; ++i) {
    state->caps[i] = resource_cap(state, (ResourceId)i) * factor;
  }
}

static void apply_haggling(GameState *state, int new_level) {
  (void)new_level;
  state->shop_discount = state->shop_discount * 0.95;
  if (state->shop_discount < 0.60) {
    state->shop_discount = 0.60;
  }
}

static void apply_salesmanship(GameState *state, int new_level) {
  (void)new_level;
  state->sell_mult = state->sell_mult * 1.10;
  if (state->sell_mult > 3.0) {
    state->sell_mult = 3.0;
  }
}

static void apply_warehouse(GameState *state, int new_level) {
  (void)new_level;
  multiply_all_caps(state, 1.15);
}

static void apply_smelters_coupon(GameState *state, int new_level) {
  (void)new_level;
  state->smelt_speed_mult *= 1.10;
}

static void apply_foreman_contract(GameState *state, int new_level) {
  (void)new_level;
  state->prod_mult *= 1.07;
}

static bool unlock_ever_iron_bar_10(const GameState *state) {
  return state && resource_ever(state, RES_IRON_BAR) >= 10.0;
}

static void apply_dedicated_crucible(GameState *state, int new_level) {
  /* Each purchase unlocks one more crucible slot (capped at MAX_CRUCIBLES). */
  if (state->crucibles_purchased < MAX_CRUCIBLES) {
    state->crucibles_purchased = new_level < MAX_CRUCIBLES ? new_level : MAX_CRUCIBLES;
  }
}

static const ShopItemDef shop_items[SHOP_COUNT] = {
    [SHOP_HAGGLING]            = {SHOP_HAGGLING,            "Haggling Lessons",    "Shop prices -5% (stacks).",             120.0,  1.8,  10, unlock_ever_coins_120,    apply_haggling},
    [SHOP_SALESMANSHIP]        = {SHOP_SALESMANSHIP,        "Salesmanship",        "Sell values +10% (stacks).",            150.0,  1.75, 12, unlock_ever_coins_150,    apply_salesmanship},
    [SHOP_WAREHOUSE]           = {SHOP_WAREHOUSE,           "Warehouse Lease",     "All storage caps +15% (stacks).",       220.0,  1.9,   8, unlock_ever_stone_200,    apply_warehouse},
    [SHOP_SMELTERS_COUPON]     = {SHOP_SMELTERS_COUPON,     "Smelter's Coupon",    "Smelting +10% speed (stacks).",         400.0,  1.85, 10, unlock_ever_iron_bar_5,   apply_smelters_coupon},
    [SHOP_FOREMAN_CONTRACT]    = {SHOP_FOREMAN_CONTRACT,    "Foreman Contract",    "All building production +7% (stacks).", 650.0,  2.0,  10, unlock_ever_bronze_5,     apply_foreman_contract},
    [SHOP_DEDICATED_CRUCIBLE]  = {SHOP_DEDICATED_CRUCIBLE,  "Dedicated Crucible",  "Unlock a crucible slot that auto-repeats one recipe forever. Up to 4.", 800.0, 3.5, MAX_CRUCIBLES, unlock_ever_iron_bar_10, apply_dedicated_crucible},
};

static const ShopItemDef *shop_item_def(ShopItemId id) {
  if ((int)id < 0 || id >= SHOP_COUNT) {
    return NULL;
  }
  return &shop_items[id];
}

static bool shop_item_unlocked(const GameState *state, const ShopItemDef *def) {
  if (!def) {
    return false;
  }
  if (!def->unlock) {
    return true;
  }
  return state && def->unlock(state);
}

static bool shop_item_maxed(const GameState *state, const ShopItemDef *def) {
  if (!state || !def) {
    return true;
  }
  if (def->max_level <= 0) {
    return false;
  }
  return shop_item_level(state, def->id) >= def->max_level;
}

static double shop_item_next_cost(const GameState *state, const ShopItemDef *def) {
  if (!state || !def) {
    return 0.0;
  }
  int lvl = shop_item_level(state, def->id);
  double raw = def->base_cost * pow(def->cost_scale, (double)lvl);
  double cost = raw * state->shop_discount;
  return ceil(cost);
}

bool buy_shop_item(GameState *state, ShopItemId id) {
  if (!state) {
    return false;
  }
  const ShopItemDef *def = shop_item_def(id);
  if (!def || !shop_item_unlocked(state, def) || shop_item_maxed(state, def)) {
    return false;
  }
  double cost = shop_item_next_cost(state, def);
  if (!resource_has(state, RES_COINS, cost)) {
    return false;
  }

  (void)resource_sub(state, RES_COINS, cost);
  int new_level = shop_item_level(state, id) + 1;
  state->shop_purchases[id] = new_level;
  if (def->apply) {
    def->apply(state, new_level);
  }
  recompute_production(state);

  char msg[256];
  (void)snprintf(msg, sizeof(msg), "Shop: %s", def->name);
  notifs_add(state, msg, 3.0f, PAL_GOLD);
  return true;
}

