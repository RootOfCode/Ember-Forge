#include "ef_recipes.h"
#include "ef_buildings.h"
#include "ef_content.h"
#include "ef_format.h"
#include "ef_state.h"
#include "ef_util.h"
#include <stdio.h>

static RecipeDef recipe_defs[RCP_COUNT];

#define RECIPE(eid_, name_, unlock_fn, dur, in_spec, out_spec)                \
    do {                                                                       \
        static const ResAmount _in[]  = {in_spec};                            \
        static const ResAmount _out[] = {out_spec};                           \
        recipe_defs[eid_] = (RecipeDef){                                      \
            .id               = eid_,                                          \
            .name             = name_,                                         \
            .inputs           = _in,                                           \
            .input_count      = EF_ARRAY_COUNT(_in),                          \
            .outputs          = _out,                                          \
            .output_count     = EF_ARRAY_COUNT(_out),                         \
            .duration_seconds = (float)(dur),                                  \
            .unlock           = unlock_fn,                                     \
        };                                                                     \
    } while (0)

/* ── unlock predicates ──────────────────────────────────────────────────── */
static bool ul_bronze     (const GameState *s){
    return s && resource_has(s,RES_COPPER_BAR,1.0) && resource_has(s,RES_TIN,1.0); }
static bool ul_steel      (const GameState *s){ return s && s->upgrades[UPG_STEEL_ALLOY]; }
static bool ul_mach_gears (const GameState *s){ return s && resource_ever(s,RES_STEEL)>=1.0; }
static bool ul_rivets     (const GameState *s){ return s && s->upgrades[UPG_METALWORKING]; }
static bool ul_steel_plate(const GameState *s){ return s && s->upgrades[UPG_METALWORKING]; }
static bool ul_mach_part  (const GameState *s){ return s && s->upgrades[UPG_INDUSTRIAL_TOOLS]; }
static bool ul_coin_mint  (const GameState *s){ return s && s->upgrades[UPG_COIN_MINTING]; }
static bool ul_ember      (const GameState *s){ return s && building_count(s,BLD_MYTHRIL_DRILL)>=1; }

/* ── recipes_init — declare all recipes here ─────────────────────────────
 * To add: enum in ef_defs.h, key in ef_defs.c, call here. */
void recipes_init(void) {
    RECIPE(RCP_IRON_BAR,     "Smelt Iron Bar",        NULL,         4.0,
        INPUTS({RES_IRON,3.0},{RES_COAL,1.0}),   OUTPUTS({RES_IRON_BAR,1.0}));
    RECIPE(RCP_COPPER_BAR,   "Smelt Copper Bar",      NULL,         4.0,
        INPUTS({RES_COPPER,3.0},{RES_COAL,1.0}), OUTPUTS({RES_COPPER_BAR,1.0}));
    RECIPE(RCP_BRONZE,       "Alloy Bronze",          ul_bronze,    8.0,
        INPUTS({RES_COPPER_BAR,2.0},{RES_TIN,1.0}), OUTPUTS({RES_BRONZE,1.0}));
    RECIPE(RCP_STEEL,        "Forge Steel",           ul_steel,    12.0,
        INPUTS({RES_IRON_BAR,2.0},{RES_COAL,3.0}),  OUTPUTS({RES_STEEL,1.0}));
    RECIPE(RCP_GEAR,         "Cast Gear",             NULL,         3.0,
        INPUTS({RES_IRON_BAR,1.0}),              OUTPUTS({RES_GEAR,1.0}));
    RECIPE(RCP_BELLOWS_PART, "Craft Bellows",         NULL,        10.0,
        INPUTS({RES_BRONZE,2.0},{RES_COAL,2.0}), OUTPUTS({RES_BELLOWS,1.0}));
    RECIPE(RCP_MACHINED_GEARS,"Machine Gears",        ul_mach_gears,6.0,
        INPUTS({RES_STEEL,1.0}),                 OUTPUTS({RES_GEAR,3.0}));
    RECIPE(RCP_RIVETS,       "Forge Rivets",          ul_rivets,    6.0,
        INPUTS({RES_IRON_BAR,1.0},{RES_COAL,1.0}),  OUTPUTS({RES_RIVET,8.0}));
    RECIPE(RCP_STEEL_PLATE,  "Hammer Steel Plate",    ul_steel_plate,14.0,
        INPUTS({RES_STEEL,2.0},{RES_COAL,1.0}),     OUTPUTS({RES_STEEL_PLATE,1.0}));
    RECIPE(RCP_MACHINE_PART, "Assemble Machine Part", ul_mach_part,22.0,
        INPUTS({RES_GEAR,2.0},{RES_STEEL_PLATE,1.0},{RES_RIVET,10.0}),
        OUTPUTS({RES_MACHINE_PART,1.0}));
    RECIPE(RCP_COIN_MINT,    "Mint Trade Tokens",     ul_coin_mint,10.0,
        INPUTS({RES_BRONZE,1.0},{RES_GEAR,1.0}), OUTPUTS({RES_COINS,250.0}));
    RECIPE(RCP_EMBER_CRYSTAL,"Condense Ember Crystal",ul_ember,    30.0,
        INPUTS({RES_MYTHRIL,10.0},{RES_STEEL,5.0}),  OUTPUTS({RES_EMBER,1.0}));
}

#undef RECIPE

/* ── accessor API ───────────────────────────────────────────────────────── */
const RecipeDef *recipe_def(RecipeId id) {
    if ((int)id < 0 || id >= RCP_COUNT) return NULL;
    return &recipe_defs[id];
}
const char *recipe_name(RecipeId id) {
    const RecipeDef *d = recipe_def(id); return d ? d->name : "???";
}
bool recipe_available(const GameState *state, const RecipeDef *def) {
    if (!def) return false;
    if (!def->unlock) return true;
    return state && def->unlock(state);
}
bool can_afford_recipe(const GameState *state, const RecipeDef *def) {
    if (!def) return false;
    return can_afford_list(state, def->inputs, def->input_count);
}
bool enqueue_smelt_job(GameState *state, RecipeId recipe_id, SmeltSource source) {
    if (!state) return false;
    const RecipeDef *def = recipe_def(recipe_id);
    if (!def || !recipe_available(state, def)) return false;
    if ((int)state->smelt_queue.len >= max_smelt_slots(state)) return false;
    if (!can_afford_recipe(state, def)) return false;
    spend_list(state, def->inputs, def->input_count);
    SmeltJob job = {.recipe=recipe_id,.progress=0.0f,
                    .duration=def->duration_seconds,.source=source};
    if (!smelt_queue_insert_front(&state->smelt_queue, job)) return false;
    char msg[256];
    (void)snprintf(msg, sizeof(msg), "Queued: %s", def->name);
    notifs_add(state, msg, 3.0f, PAL_TEXT);
    return true;
}
bool cancel_smelt_job(GameState *state, size_t index) {
    if (!state || index >= state->smelt_queue.len) return false;
    SmeltJob job = state->smelt_queue.items[index];
    const RecipeDef *def = recipe_def(job.recipe);
    if (def) {
        for (size_t i = 0; i < def->input_count; ++i)
            (void)resource_add(state, def->inputs[i].res, def->inputs[i].amount);
        notifs_add(state, "Canceled smelt job", 3.0f, PAL_WARN);
    }
    return smelt_queue_remove_at(&state->smelt_queue, index);
}
void tick_smelt_queue(GameState *state, float dt_seconds) {
    if (!state) return;
    double speed = smelt_speed_multiplier(state);
    size_t out_len = 0;
    for (size_t i = 0; i < state->smelt_queue.len; ++i) {
        SmeltJob job = state->smelt_queue.items[i];
        float dur = job.duration > 0.01f ? job.duration : 0.01f;
        job.progress += (float)(((double)dt_seconds * speed) / (double)dur);
        if (job.progress >= 1.0f) {
            const RecipeDef *def = recipe_def(job.recipe);
            if (def) {
                double mult = 1.0;
                if (state->upgrades[UPG_PRECISION_MOLDS])
                    if (def->id==RCP_GEAR || def->id==RCP_BELLOWS_PART || def->id==RCP_RIVETS)
                        mult = 2.0;
                for (size_t j = 0; j < def->output_count; ++j)
                    (void)resource_add(state, def->outputs[j].res, def->outputs[j].amount * mult);
                char msg[256];
                (void)snprintf(msg, sizeof(msg), "Completed: %s", def->name);
                notifs_add(state, msg, 3.0f, PAL_GREEN_OK);
            }
            continue;
        }
        state->smelt_queue.items[out_len++] = job;
    }
    state->smelt_queue.len = out_len;
}
void tick_auto_furnaces(GameState *state) {
    if (!state) return;
    int desired = building_count(state, BLD_FURNACE);
    int running = 0;
    for (size_t i = 0; i < state->smelt_queue.len; ++i)
        if (state->smelt_queue.items[i].source==SMELT_SOURCE_AUTO &&
            state->smelt_queue.items[i].recipe==RCP_IRON_BAR) running++;
    while (running < desired && (int)state->smelt_queue.len < max_smelt_slots(state)) {
        if (enqueue_smelt_job(state, RCP_IRON_BAR, SMELT_SOURCE_AUTO)) running++;
        else break;
    }
}

/* ── Dedicated Crucibles ─────────────────────────────────────────────────── */

void crucible_set_recipe(GameState *state, int slot, RecipeId recipe) {
    if (!state || slot < 0 || slot >= state->crucibles_purchased) return;
    if (slot >= MAX_CRUCIBLES) return;
    CrucibleSlot *c = &state->crucibles[slot];
    /* stop current run, no refund (not mid-job spending like the queue) */
    c->has_recipe = true;
    c->recipe     = recipe;
    c->progress   = 0.0f;
    c->stalled    = false;
    const RecipeDef *def = recipe_def(recipe);
    c->duration   = def ? def->duration_seconds : 4.0f;
}

void crucible_start(GameState *state, int slot) {
    if (!state || slot < 0 || slot >= state->crucibles_purchased) return;
    if (slot >= MAX_CRUCIBLES) return;
    CrucibleSlot *c = &state->crucibles[slot];
    if (!c->has_recipe) return;
    const RecipeDef *def = recipe_def(c->recipe);
    if (!def || !recipe_available(state, def)) return;
    if (!can_afford_recipe(state, def)) { c->stalled = true; return; }
    spend_list(state, def->inputs, def->input_count);
    c->progress = 0.0f;
    c->stalled  = false;
}

void crucible_stop(GameState *state, int slot) {
    if (!state || slot < 0 || slot >= MAX_CRUCIBLES) return;
    CrucibleSlot *c = &state->crucibles[slot];
    c->has_recipe = false;
    c->stalled    = false;
    c->progress   = 0.0f;
}

void tick_crucibles(GameState *state, float dt_seconds) {
    if (!state) return;
    double speed = smelt_speed_multiplier(state);
    int n = state->crucibles_purchased < MAX_CRUCIBLES
            ? state->crucibles_purchased : MAX_CRUCIBLES;
    for (int i = 0; i < n; ++i) {
        CrucibleSlot *c = &state->crucibles[i];
        if (!c->has_recipe) continue;

        const RecipeDef *def = recipe_def(c->recipe);
        if (!def || !recipe_available(state, def)) { c->stalled = true; continue; }

        /* If stalled, try to re-start each tick */
        if (c->stalled) {
            if (!can_afford_recipe(state, def)) continue;
            spend_list(state, def->inputs, def->input_count);
            c->progress = 0.0f;
            c->stalled  = false;
        }

        float dur = c->duration > 0.01f ? c->duration : 0.01f;
        c->progress += (float)(((double)dt_seconds * speed) / (double)dur);

        if (c->progress >= 1.0f) {
            /* deliver outputs */
            double mult = 1.0;
            if (state->upgrades[UPG_PRECISION_MOLDS])
                if (def->id==RCP_GEAR || def->id==RCP_BELLOWS_PART || def->id==RCP_RIVETS)
                    mult = 2.0;
            for (size_t j = 0; j < def->output_count; ++j)
                (void)resource_add(state, def->outputs[j].res, def->outputs[j].amount * mult);

            /* auto-restart: spend inputs immediately */
            if (can_afford_recipe(state, def)) {
                spend_list(state, def->inputs, def->input_count);
                c->progress = 0.0f;
                c->stalled  = false;
            } else {
                c->progress = 0.0f;
                c->stalled  = true;
            }
        }
    }
}
