#pragma once

#include "ef_defs.h"

#include <stdbool.h>

typedef struct GameState GameState;

double shop_buy_mult(const GameState *state);
double shop_buy_cost(const GameState *state, ResourceId resource, double qty);
bool shop_can_buy_resource(const GameState *state, ResourceId resource, double qty);
bool buy_resource(GameState *state, ResourceId resource, double qty);

int shop_item_level(const GameState *state, ShopItemId id);
bool shop_item_owned(const GameState *state, ShopItemId id);

bool buy_shop_item(GameState *state, ShopItemId id);

