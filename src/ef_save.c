/*
 * ef_save.c — Save / load system (pure C, line-based key=value format)
 *
 * Save file  (.sav):  saves/slotN/save.sav
 * Options    (.sav):  saves/options.sav
 *
 * Format:
 *   version=4
 *   saved_at=<unix seconds>
 *   resource.stone=450.0
 *   cap.stone=500.0
 *   building.pickaxe=2 1
 *   upgrades=quick_hands smelt_slot_1
 *   smelt_queue.0=iron_bar 0.0 4.0 manual
 *   active_panel=forge
 *
 * Old .lisp saves are accepted as a read-only fallback.
 */

#include "ef_save.h"
#include "ef_platform.h"

#include "ef_buildings.h"
#include "ef_defs.h"
#include "ef_recipes.h"
#include "ef_resources.h"
#include "ef_state.h"
#include "ef_time.h"
#include "ef_util.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Path helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static int normalize_slot(int slot) {
    return slot < 1 ? 1 : slot > 3 ? 3 : slot;
}
static int normalize_interval(int iv) {
    return iv < 10 ? 10 : iv > 3600 ? 3600 : iv;
}

static void make_save_dir(char *out, size_t n, int slot) {
    (void)snprintf(out, n, "saves" EF_PATH_SEP "slot%d", normalize_slot(slot));
}
static void make_save_path(char *out, size_t n, int slot) {
    (void)snprintf(out, n, "saves" EF_PATH_SEP "slot%d" EF_PATH_SEP "save.sav",
                   normalize_slot(slot));
}
static void make_legacy_path(char *out, size_t n, int slot) {
    (void)snprintf(out, n, "saves" EF_PATH_SEP "slot%d" EF_PATH_SEP "save.lisp",
                   normalize_slot(slot));
}
static void make_legacy2_path(char *out, size_t n, int slot) {
    int s = normalize_slot(slot);
    if (s == 1) (void)snprintf(out, n, "saves" EF_PATH_SEP "save.lisp");
    else        (void)snprintf(out, n, "saves" EF_PATH_SEP "save%d.lisp", s);
}
static const char *options_path(void) {
    const char *env = getenv("EMBER_FORGE_OPTIONS_PATH");
    if (env && env[0]) return env;
    return "saves" EF_PATH_SEP "options.sav";
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Writer — save_game
 * ═══════════════════════════════════════════════════════════════════════════ */

static void write_bool_list(FILE *f, const char *key,
                             const bool *arr, int count,
                             const char *(*key_fn)(int)) {
    fprintf(f, "%s=", key);
    bool first = true;
    for (int i = 0; i < count; ++i) {
        if (!arr[i]) continue;
        if (!first) fputc(' ', f);
        fputs(key_fn(i), f);
        first = false;
    }
    fputc('\n', f);
}

static const char *res_key_i(int i)  { return resource_key((ResourceId)i); }
static const char *upg_key_i(int i)  { return upgrade_key((UpgradeId)i); }
static const char *eter_key_i(int i) { return eternal_upgrade_key((EternalUpgradeId)i); }
static const char *ach_key_i(int i)  { return achievement_key((AchievementId)i); }

bool save_game(GameState *state) {
    if (!state) return false;

    int slot = normalize_slot(state->save_slot);
    (void)ef_ensure_dir("saves");
    char dir[256]; make_save_dir(dir, sizeof(dir), slot);
    (void)ef_ensure_dir(dir);
    char path[256]; make_save_path(path, sizeof(path), slot);

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint64_t now = universal_time_now();

    fprintf(f, "version=1\n");
    fprintf(f, "saved_at=%llu\n", (unsigned long long)now);
    fprintf(f, "tick=%llu\n",     (unsigned long long)state->tick);

    for (int i = 0; i < RES_COUNT; ++i)
        fprintf(f, "resource.%s=%.17g\n", resource_key((ResourceId)i), state->resources[i]);
    for (int i = 0; i < RES_COUNT; ++i)
        fprintf(f, "cap.%s=%.17g\n",      resource_key((ResourceId)i), state->caps[i]);
    for (int i = 0; i < RES_COUNT; ++i)
        fprintf(f, "ever.%s=%.17g\n",     resource_key((ResourceId)i), state->ever_produced[i]);

    write_bool_list(f, "unlocked", state->unlocked, RES_COUNT, res_key_i);

    for (int i = 0; i < BLD_COUNT; ++i)
        fprintf(f, "building.%s=%d %d\n",
                building_key((BuildingId)i),
                state->buildings[i].count, state->buildings[i].level);
    for (int i = 0; i < BLD_COUNT; ++i)
        fprintf(f, "bld_mult.%s=%.17g\n",
                building_key((BuildingId)i), state->building_mults[i]);

    fprintf(f, "smelt_queue_len=%zu\n", state->smelt_queue.len);
    for (size_t i = 0; i < state->smelt_queue.len; ++i) {
        SmeltJob j = state->smelt_queue.items[i];
        fprintf(f, "smelt_queue.%zu=%s %.8f %.8f %s\n",
                i, recipe_key(j.recipe),
                (double)j.progress, (double)j.duration,
                smelt_source_key(j.source));
    }

    write_bool_list(f, "upgrades",         state->upgrades,         UPG_COUNT,     upg_key_i);
    write_bool_list(f, "eternal_upgrades", state->eternal_upgrades, ETERNAL_COUNT, eter_key_i);
    write_bool_list(f, "achievements",     state->achievements,     ACH_COUNT,     ach_key_i);

    for (int i = 0; i < RES_COUNT; ++i)
        if (state->milestones_fired[i])
            fprintf(f, "milestone.%s=%u\n",
                    resource_key((ResourceId)i), (unsigned)state->milestones_fired[i]);

    for (int i = 0; i < SHOP_COUNT; ++i)
        fprintf(f, "shop_lvl.%s=%d\n", shop_item_key((ShopItemId)i), state->shop_purchases[i]);

    fprintf(f, "sell_mult=%.17g\n",              state->sell_mult);
    fprintf(f, "shop_discount=%.17g\n",           state->shop_discount);
    fprintf(f, "essence=%.17g\n",                 state->essence);
    fprintf(f, "ascensions=%d\n",                 state->ascensions);
    fprintf(f, "prod_mult=%.17g\n",               state->prod_mult);
    fprintf(f, "click_mult=%.17g\n",              state->click_mult);
    fprintf(f, "gather_mult=%.17g\n",             state->gather_mult);
    fprintf(f, "smelt_speed_mult=%.17g\n",        state->smelt_speed_mult);
    fprintf(f, "manual_mine_duration=%.8f\n",     (double)state->manual_mine_duration);
    fprintf(f, "manual_gather_duration=%.8f\n",   (double)state->manual_gather_duration);
    fprintf(f, "event_timer=%.8f\n",              (double)state->event_timer);
    fprintf(f, "smelt_boost_timer=%.8f\n",        (double)state->smelt_boost_timer);
    fprintf(f, "iron_penalty_timer=%.8f\n",       (double)state->iron_penalty_timer);
    fprintf(f, "active_panel=%s\n",               panel_key(state->active_panel));
    fprintf(f, "smelt_slots_extra=%d\n",          state->smelt_slots_extra);

    fprintf(f, "crucibles_purchased=%d\n", state->crucibles_purchased);
    for (int i = 0; i < MAX_CRUCIBLES; ++i) {
        const CrucibleSlot *c = &state->crucibles[i];
        if (c->has_recipe) {
            fprintf(f, "crucible.%d=%s %.8f %d\n",
                    i, recipe_key(c->recipe),
                    (double)c->progress, (int)c->stalled);
        }
    }

    fclose(f);
    state->last_save_universal = now;
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Reader — load_save
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct { char key[256]; char value[1024]; } KVLine;

static bool parse_line(const char *line, KVLine *out) {
    if (!line || !out) return false;
    while (*line == ' ' || *line == '\t') ++line;
    if (*line == '#' || *line == '\0' || *line == '\n' || *line == '\r') return false;
    const char *eq = strchr(line, '=');
    if (!eq) return false;
    size_t klen = (size_t)(eq - line);
    if (klen == 0 || klen >= sizeof(out->key)) return false;
    memcpy(out->key, line, klen); out->key[klen] = '\0';
    const char *vs = eq + 1;
    size_t vlen = strlen(vs);
    while (vlen > 0 && (vs[vlen-1]=='\r' || vs[vlen-1]=='\n')) --vlen;
    if (vlen >= sizeof(out->value)) vlen = sizeof(out->value) - 1;
    memcpy(out->value, vs, vlen); out->value[vlen] = '\0';
    return true;
}

static double  parse_double(const char *s) {
    if (!s || !*s) return 0.0;
    char *e = NULL; errno = 0;
    double v = strtod(s, &e);
    return (errno == 0 && e != s) ? v : 0.0;
}
static int     parse_int(const char *s) {
    if (!s || !*s) return 0;
    char *e = NULL; errno = 0;
    long v = strtol(s, &e, 10);
    return (errno == 0 && e != s) ? (int)v : 0;
}
static uint64_t parse_u64(const char *s) {
    if (!s || !*s) return 0;
    char *e = NULL; errno = 0;
    unsigned long long v = strtoull(s, &e, 10);
    return (errno == 0 && e != s) ? (uint64_t)v : 0;
}

typedef void (*TokenCb)(const char *, void *);
static void each_token(const char *src, TokenCb cb, void *ctx) {
    if (!src) return;
    char buf[1024];
    size_t n = strlen(src); if (n >= sizeof(buf)) n = sizeof(buf)-1;
    memcpy(buf, src, n); buf[n] = '\0';
    char *p = buf;
    while (*p) {
        while (*p==' '||*p=='\t') ++p;
        if (!*p) break;
        char *start = p;
        while (*p && *p!=' ' && *p!='\t') ++p;
        if (*p) *p++ = '\0';
        cb(start, ctx);
    }
}

static void tok_unlocked   (const char *t, void *c){ GameState *s=c; ResourceId r; if(resource_from_key(t,&r)) s->unlocked[r]=true; }
static void tok_upgrade    (const char *t, void *c){ GameState *s=c; UpgradeId u; if(upgrade_from_key(t,&u)) s->upgrades[u]=true; }
static void tok_eternal    (const char *t, void *c){ GameState *s=c; EternalUpgradeId e; if(eternal_upgrade_from_key(t,&e)) s->eternal_upgrades[e]=true; }
static void tok_achievement(const char *t, void *c){ GameState *s=c; AchievementId a; if(achievement_from_key(t,&a)) s->achievements[a]=true; }

static void apply_kv(GameState *state, const KVLine *kv,
                     SmeltJob *qbuf, size_t qcap, size_t *qlen) {
    const char *k = kv->key, *v = kv->value;

    if      (!strcmp(k,"saved_at"))               state->last_save_universal = parse_u64(v);
    else if (!strcmp(k,"tick"))                   state->tick                 = parse_u64(v);
    else if (!strcmp(k,"sell_mult"))              state->sell_mult            = parse_double(v);
    else if (!strcmp(k,"shop_discount"))          state->shop_discount        = parse_double(v);
    else if (!strcmp(k,"essence"))                state->essence              = parse_double(v);
    else if (!strcmp(k,"ascensions"))             state->ascensions           = parse_int(v);
    else if (!strcmp(k,"prod_mult"))              state->prod_mult            = parse_double(v);
    else if (!strcmp(k,"click_mult"))             state->click_mult           = parse_double(v);
    else if (!strcmp(k,"gather_mult"))            state->gather_mult          = parse_double(v);
    else if (!strcmp(k,"smelt_speed_mult"))       state->smelt_speed_mult     = parse_double(v);
    else if (!strcmp(k,"manual_mine_duration"))   state->manual_mine_duration   = (float)parse_double(v);
    else if (!strcmp(k,"manual_gather_duration")) state->manual_gather_duration = (float)parse_double(v);
    else if (!strcmp(k,"event_timer"))            state->event_timer            = (float)parse_double(v);
    else if (!strcmp(k,"smelt_boost_timer"))      state->smelt_boost_timer      = (float)parse_double(v);
    else if (!strcmp(k,"iron_penalty_timer"))     state->iron_penalty_timer     = (float)parse_double(v);
    else if (!strcmp(k,"smelt_slots_extra"))      state->smelt_slots_extra      = parse_int(v);
    else if (!strcmp(k,"crucibles_purchased"))    state->crucibles_purchased    = parse_int(v);
    else if (!strncmp(k,"crucible.",9)) {
        int slot = parse_int(k+9);
        if (slot >= 0 && slot < MAX_CRUCIBLES) {
            char rkey[64]={0}; double progress=0; int stalled=0;
            int n = sscanf(v, "%63s %lf %d", rkey, &progress, &stalled);
            if (n >= 1) {
                RecipeId rid;
                if (recipe_from_key(rkey, &rid)) {
                    CrucibleSlot *c = &state->crucibles[slot];
                    c->has_recipe = true;
                    c->recipe     = rid;
                    c->progress   = (float)progress;
                    c->stalled    = stalled != 0;
                    const RecipeDef *def = recipe_def(rid);
                    c->duration   = def ? def->duration_seconds : 4.0f;
                }
            }
        }
    }
    else if (!strcmp(k,"active_panel")) {
        Panel p; if (panel_from_key(v, &p)) state->active_panel = p;
    }
    else if (!strcmp(k,"unlocked"))         { memset(state->unlocked,0,sizeof(state->unlocked));         each_token(v,tok_unlocked,   state); }
    else if (!strcmp(k,"upgrades"))         { memset(state->upgrades,0,sizeof(state->upgrades));         each_token(v,tok_upgrade,    state); }
    else if (!strcmp(k,"eternal_upgrades")) { memset(state->eternal_upgrades,0,sizeof(state->eternal_upgrades)); each_token(v,tok_eternal,state); }
    else if (!strcmp(k,"achievements"))     { memset(state->achievements,0,sizeof(state->achievements)); each_token(v,tok_achievement,state); }
    else if (!strncmp(k,"resource.",9))  { ResourceId r; if(resource_from_key(k+9,&r))  state->resources[r]    = parse_double(v); }
    else if (!strncmp(k,"cap.",4))       { ResourceId r; if(resource_from_key(k+4,&r))  state->caps[r]         = parse_double(v); }
    else if (!strncmp(k,"ever.",5))      { ResourceId r; if(resource_from_key(k+5,&r))  state->ever_produced[r]= parse_double(v); }
    else if (!strncmp(k,"milestone.",10)) {
        ResourceId r; if(resource_from_key(k+10,&r)) state->milestones_fired[r]=(uint8_t)parse_int(v);
    }
    else if (!strncmp(k,"building.",9))  {
        BuildingId b; if(building_from_key(k+9,&b)) {
            int count=0,level=1; (void)sscanf(v,"%d %d",&count,&level);
            state->buildings[b].count=count; state->buildings[b].level=level;
        }
    }
    else if (!strncmp(k,"bld_mult.",9))  { BuildingId b; if(building_from_key(k+9,&b)) state->building_mults[b]=parse_double(v); }
    else if (!strncmp(k,"shop_lvl.",9))  { ShopItemId s; if(shop_item_from_key(k+9,&s)) state->shop_purchases[s]=parse_int(v); }
    else if (!strncmp(k,"smelt_queue.",12)) {
        if (qbuf && qlen && *qlen < qcap) {
            char rkey[64]={0}, skey[32]={0}; double progress=0,duration=0;
            int n = sscanf(v,"%63s %lf %lf %31s", rkey, &progress, &duration, skey);
            if (n >= 3) {
                RecipeId rid; if (recipe_from_key(rkey,&rid)) {
                    SmeltSource src = SMELT_SOURCE_MANUAL;
                    if (n >= 4) smelt_source_from_key(skey,&src);
                    qbuf[(*qlen)++] = (SmeltJob){rid,(float)progress,(float)duration,src};
                }
            }
        }
    }
}

static bool read_file(const char *path, char **out) {
    if (!path || !out) return false;
    *out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f,0,SEEK_END)!=0) { fclose(f); return false; }
    long sz = ftell(f); if (sz < 0) { fclose(f); return false; }
    rewind(f);
    char *buf = (char*)malloc((size_t)sz+1);
    if (!buf) { fclose(f); return false; }
    size_t n = fread(buf,1,(size_t)sz,f);
    fclose(f); buf[n]='\0'; *out=buf; return true;
}

static bool parse_save_buf(GameState *state, const char *buf) {
    if (!state || !buf) return false;
    SmeltJob q_buf[64]; size_t q_len=0;
    const char *p = buf;
    while (*p) {
        const char *eol = p; while (*eol && *eol!='\n') ++eol;
        size_t ll = (size_t)(eol-p); char line[1280];
        if (ll >= sizeof(line)) ll = sizeof(line)-1;
        memcpy(line,p,ll); line[ll]='\0';
        KVLine kv; if (parse_line(line,&kv)) apply_kv(state,&kv,q_buf,EF_ARRAY_COUNT(q_buf),&q_len);
        p = (*eol=='\n') ? eol+1 : eol;
    }
    state->smelt_queue.len = 0;
    for (size_t i=0; i<q_len; ++i) {
        if (state->smelt_queue.len >= state->smelt_queue.cap) {
            size_t nc = state->smelt_queue.cap>0 ? state->smelt_queue.cap*2 : 16;
            void *nm = realloc(state->smelt_queue.items, nc*sizeof(SmeltJob));
            if (!nm) break;
            state->smelt_queue.items = (SmeltJob*)nm;
            state->smelt_queue.cap   = nc;
        }
        state->smelt_queue.items[state->smelt_queue.len++] = q_buf[i];
    }
    return true;
}

bool load_save(GameState *state) {
    if (!state) return false;
    int slot = normalize_slot(state->save_slot);
    char path[256]; make_save_path(path, sizeof(path), slot);
    char leg1[256]; make_legacy_path(leg1, sizeof(leg1), slot);
    char leg2[256]; make_legacy2_path(leg2, sizeof(leg2), slot);
    const char *candidates[] = {path, leg1, leg2};
    for (size_t ci=0; ci<EF_ARRAY_COUNT(candidates); ++ci) {
        if (!ef_path_exists(candidates[ci])) continue;
        char *buf=NULL;
        if (!read_file(candidates[ci],&buf)) continue;
        bool ok = parse_save_buf(state, buf);
        free(buf);
        if (ok) { state->active_event=EVENT_NONE; return true; }
    }
    return false;
}

bool save_exists(const GameState *state, int slot) {
    (void)state;
    char path[256]; make_save_path(path, sizeof(path), slot);
    char leg1[256]; make_legacy_path(leg1, sizeof(leg1), slot);
    char leg2[256]; make_legacy2_path(leg2, sizeof(leg2), slot);
    return ef_path_exists(path) || ef_path_exists(leg1) || ef_path_exists(leg2);
}

bool delete_save(GameState *state, int slot) {
    (void)state;
    char path[256]; make_save_path(path, sizeof(path), slot);
    char dir[256];  make_save_dir(dir, sizeof(dir), slot);
    char leg1[256]; make_legacy_path(leg1, sizeof(leg1), slot);
    bool deleted = false;
    if (ef_path_exists(path)) deleted |= (remove(path)==0);
    (void)ef_rmdir(dir);
    if (!deleted && ef_path_exists(leg1)) deleted |= (remove(leg1)==0);
    return deleted;
}

uint64_t save_slot_saved_at(int slot) {
    char path[256]; make_save_path(path, sizeof(path), slot);
    char leg1[256]; make_legacy_path(leg1, sizeof(leg1), slot);
    char leg2[256]; make_legacy2_path(leg2, sizeof(leg2), slot);
    const char *candidates[] = {path, leg1, leg2};
    for (size_t ci = 0; ci < EF_ARRAY_COUNT(candidates); ++ci) {
        if (!ef_path_exists(candidates[ci])) continue;
        FILE *f = fopen(candidates[ci], "rb");
        if (!f) continue;
        char line[512];
        uint64_t result = 0;
        while (fgets(line, sizeof(line), f)) {
            KVLine kv;
            if (parse_line(line, &kv) && strcmp(kv.key, "saved_at") == 0) {
                result = parse_u64(kv.value);
                break;
            }
        }
        fclose(f);
        return result;
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Options
 * ═══════════════════════════════════════════════════════════════════════════ */

bool save_options(const GameState *state) {
    if (!state) return false;
    (void)ef_ensure_dir("saves");
    FILE *f = fopen(options_path(),"wb");
    if (!f) f = fopen("ember-forge.sav","wb");
    if (!f) return false;
    fprintf(f,"version=4\n");
    fprintf(f,"saved_at=%llu\n",   (unsigned long long)universal_time_now());
    fprintf(f,"save_slot=%d\n",    normalize_slot(state->save_slot));
    fprintf(f,"autosave=%d\n",     state->autosave_enabled ? 1 : 0);
    fprintf(f,"autosave_interval=%d\n", normalize_interval(state->autosave_interval_seconds));
    fprintf(f,"offline_progress=%d\n",  state->offline_progress_enabled ? 1 : 0);
    fprintf(f,"fullscreen=%d\n",   state->fullscreen_enabled ? 1 : 0);
    fprintf(f,"font_path=%s\n",    state->font_path);
    for (int i = 0; i < 3; ++i)
        if (state->slot_names[i][0])
            fprintf(f,"slot_name.%d=%s\n", i + 1, state->slot_names[i]);
    fclose(f);
    return true;
}

static void apply_options_kv(GameState *state, const KVLine *kv) {
    const char *k=kv->key, *v=kv->value;
    if      (!strcmp(k,"save_slot"))         state->save_slot                = normalize_slot(parse_int(v));
    else if (!strcmp(k,"autosave"))          state->autosave_enabled         = parse_int(v)!=0;
    else if (!strcmp(k,"autosave_interval")) state->autosave_interval_seconds= normalize_interval(parse_int(v));
    else if (!strcmp(k,"offline_progress"))  state->offline_progress_enabled = parse_int(v)!=0;
    else if (!strcmp(k,"fullscreen"))        state->fullscreen_enabled       = parse_int(v)!=0;
    else if (!strcmp(k,"font_path")) {
        strncpy(state->font_path,v,sizeof(state->font_path)-1);
        state->font_path[sizeof(state->font_path)-1]='\0';
    }
    else if (!strncmp(k,"slot_name.",10)) {
        int s = parse_int(k + 10);
        if (s >= 1 && s <= 3) {
            size_t cap = sizeof(state->slot_names[0]) - 1;
            size_t len = strlen(v);
            if (len > cap) len = cap;
            memcpy(state->slot_names[s-1], v, len);
            state->slot_names[s-1][len] = '\0';
        }
    }
}

bool load_options(GameState *state) {
    if (!state) return false;
    const char *paths[] = {options_path(),"ember-forge.sav",
                           "saves" EF_PATH_SEP "options.lisp","ember-forge-options.lisp"};
    for (size_t i=0; i<EF_ARRAY_COUNT(paths); ++i) {
        if (!ef_path_exists(paths[i])) continue;
        char *buf=NULL; if (!read_file(paths[i],&buf)) continue;
        const char *p=buf;
        while (*p) {
            const char *eol=p; while (*eol && *eol!='\n') ++eol;
            size_t ll=(size_t)(eol-p); char line[1280];
            if (ll>=sizeof(line)) ll=sizeof(line)-1;
            memcpy(line,p,ll); line[ll]='\0';
            KVLine kv; if (parse_line(line,&kv)) apply_options_kv(state,&kv);
            p=(*eol=='\n') ? eol+1 : eol;
        }
        free(buf); return true;
    }
    return false;
}
