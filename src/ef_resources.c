#include "ef_resources.h"

#include "ef_content.h"
#include "ef_format.h"
#include "ef_math.h"
#include "ef_milestones.h"
#include "ef_state.h"

#include <stdio.h>

/* ── resource definition table (built from EF_EACH_RESOURCE X-macro) ───── */
#define X(id, key, name, color, tier, base_cap, sell, gather_time) \
    [id] = {name, color, tier, base_cap, sell, (float)(gather_time)},

static const ResourceDef resource_defs[RES_COUNT] = {
    EF_EACH_RESOURCE(X)
};

#undef X

/* ── accessors ──────────────────────────────────────────────────────────── */
const ResourceDef *resource_def(ResourceId r) {
    if ((int)r < 0 || r >= RES_COUNT) return NULL;
    return &resource_defs[r];
}
const char *resource_name(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->name : "???";
}
PaletteColor resource_color(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->color : PAL_TEXT;
}
int resource_tier(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->tier : 0;
}
double resource_base_cap(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->base_cap : 0.0;
}
double sell_value(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->sell_value : 0.0;
}
float resource_base_gather_time(ResourceId r) {
    const ResourceDef *d = resource_def(r); return d ? d->base_gather_time : 0.0f;
}

/* ── cap management ─────────────────────────────────────────────────────── */
void init_resource_caps(GameState *state) {
    if (!state) return;
    for (int i = 0; i < RES_COUNT; ++i)
        state->caps[i] = resource_base_cap((ResourceId)i);
}
double resource_cap(GameState *state, ResourceId r) {
    if (!state) return 0.0;
    double cap = state->caps[r];
    if (cap <= 0.0) { cap = resource_base_cap(r); state->caps[r] = cap; }
    return cap;
}

/* ── amounts ────────────────────────────────────────────────────────────── */
double resource_amount(const GameState *state, ResourceId r) {
    return state ? state->resources[r] : 0.0;
}
double resource_ever(const GameState *state, ResourceId r) {
    return state ? state->ever_produced[r] : 0.0;
}
bool resource_unlocked_p(const GameState *state, ResourceId r) {
    return state && state->unlocked[r];
}
void resource_unlock(GameState *state, ResourceId r) {
    if (state) state->unlocked[r] = true;
}
bool resource_has(const GameState *state, ResourceId r, double amount) {
    return state && state->resources[r] >= amount;
}
double resource_add(GameState *state, ResourceId r, double delta) {
    if (!state) return 0.0;
    double before = state->resources[r];
    double cap    = resource_cap(state, r);
    double after  = clamp_double(before + delta, 0.0, cap);
    state->resources[r] = after;
    if (after > 0.0) resource_unlock(state, r);
    /* Track ever_produced from the raw positive delta so milestones fire
       even when the resource bucket is full and no actual units are added. */
    if (delta > 0.0) {
        state->ever_produced[r] += delta;
        milestones_check(state, r);
    }
    return after;
}
double resource_sub(GameState *state, ResourceId r, double delta) {
    return resource_add(state, r, -delta);
}
void maybe_unlock_produced_resources(GameState *state) {
    if (!state) return;
    for (int i = 0; i < RES_COUNT; ++i)
        if (state->resources[i] > 0.0) resource_unlock(state, (ResourceId)i);
}

/* ── afford / spend ─────────────────────────────────────────────────────── */
bool can_afford_list(const GameState *state, const ResAmount *items, size_t n) {
    if (!state) return false;
    for (size_t i = 0; i < n; ++i)
        if (items[i].amount > 0.0 && !resource_has(state, items[i].res, items[i].amount))
            return false;
    return true;
}
void spend_list(GameState *state, const ResAmount *items, size_t n) {
    if (!state) return;
    for (size_t i = 0; i < n; ++i)
        if (items[i].amount > 0.0) (void)resource_sub(state, items[i].res, items[i].amount);
}

/* ── selling ────────────────────────────────────────────────────────────── */
bool can_sell(const GameState *state, ResourceId r, double qty) {
    return state && r != RES_COINS && sell_value(r) > 0.0 && resource_has(state, r, qty);
}
bool sell_resource(GameState *state, ResourceId r, double qty) {
    if (!can_sell(state, r, qty)) return false;
    double coins = sell_value(r) * qty * state->sell_mult;
    (void)resource_sub(state, r, qty);
    (void)resource_add(state, RES_COINS, coins);
    char qty_buf[64];
    fmt_number(qty_buf, sizeof(qty_buf), qty);
    char msg[256];
    (void)snprintf(msg, sizeof(msg), "Sold %s %s", qty_buf, resource_name(r));
    notifs_add(state, msg, 3.0f, PAL_GOLD);
    return true;
}
