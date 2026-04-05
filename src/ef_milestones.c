#include "ef_milestones.h"

#include "ef_format.h"
#include "ef_resources.h"
#include "ef_state.h"

#include <stddef.h>
#include <stdio.h>

/* ── Milestone threshold table ──────────────────────────────────────────────
 *
 * Each resource gets up to EF_MILESTONE_MAX thresholds, stored in ascending
 * order.  A 0.0 entry terminates the list early.
 *
 * The bitmask in state->milestones_fired[r] uses one bit per threshold slot,
 * so EF_MILESTONE_MAX must never exceed 8 (uint8_t).
 *
 * Design intent:
 *   tier 0  (Stone, Coal, Coins)   — produced fast; milestones through 10 M
 *   tier 1  (Iron/Copper/Tin ore)  — meaningful but plentiful; up to 10 k
 *   tier 2  (Iron/Copper Bar)      — smelted goods; celebrate every step
 *   tier 3  (Bronze, Gear, Bellows, Rivets)
 *   tier 4  (Steel Bar, Steel Plate)
 *   tier 5  (Machine Part, Mythril)
 *   tier 6  (Ember Crystal)        — ultra-rare; every single one counts
 */

#define EF_MILESTONE_MAX 7   /* fits in uint8_t bitmask (max 8) */

typedef struct {
    double thresholds[EF_MILESTONE_MAX]; /* ascending; 0.0 = end of list */
} MilestoneDef;

static const MilestoneDef milestone_defs[RES_COUNT] = {
    /* RES_STONE       tier 0 */ {{   100,   1000,   10000,  100000, 1000000,       0,       0 }},
    /* RES_COAL        tier 0 */ {{   100,   1000,   10000,  100000,       0,       0,       0 }},
    /* RES_COINS       tier 0 */ {{   100,   1000,   10000,  100000, 1000000, 5000000,10000000 }},
    /* RES_IRON        tier 1 */ {{    10,    100,    1000,   10000,       0,       0,       0 }},
    /* RES_COPPER      tier 1 */ {{    10,    100,    1000,   10000,       0,       0,       0 }},
    /* RES_TIN         tier 1 */ {{    10,    100,    1000,   10000,       0,       0,       0 }},
    /* RES_IRON_BAR    tier 2 */ {{     1,     10,      50,     100,     500,    1000,       0 }},
    /* RES_COPPER_BAR  tier 2 */ {{     1,     10,      50,     100,     500,    1000,       0 }},
    /* RES_BRONZE      tier 3 */ {{     1,     10,      25,      50,     100,     500,       0 }},
    /* RES_GEAR        tier 3 */ {{     1,     10,      25,      50,     100,     500,       0 }},
    /* RES_BELLOWS     tier 3 */ {{     1,     10,      25,      50,     100,       0,       0 }},
    /* RES_STEEL       tier 4 */ {{     1,      5,      20,      50,     100,     500,       0 }},
    /* RES_STEEL_PLATE tier 4 */ {{     1,      5,      20,      50,     100,     500,       0 }},
    /* RES_RIVET       tier 3 */ {{     1,     25,     100,     500,    1000,       0,       0 }},
    /* RES_MACHINE_PART tier 5*/ {{     1,      5,      10,      25,      50,     100,       0 }},
    /* RES_MYTHRIL     tier 5 */ {{     1,      5,      10,      25,      50,     100,       0 }},
    /* RES_EMBER       tier 6 */ {{     1,      3,       5,      10,      25,      50,     100 }},
};

/* Compile-time check: table must cover every ResourceId */
typedef char _milestone_table_size_check[
    (sizeof(milestone_defs) / sizeof(milestone_defs[0]) == RES_COUNT) ? 1 : -1
];

/* ── Internal helpers ───────────────────────────────────────────────────── */

static const char *milestone_label(double threshold, const char *name,
                                   char *buf, size_t buf_len) {
    if (threshold <= 1.0) {
        (void)snprintf(buf, buf_len, "\xe2\x98\x85 First %s!", name);
    } else {
        char num[32];
        fmt_number(num, sizeof(num), threshold);
        (void)snprintf(buf, buf_len, "\xe2\x98\x85 %s %s collected!", num, name);
    }
    return buf;
}

/* Display time scales with rarity: rarer resources get more screen time */
static float milestone_duration(double threshold, ResourceId r) {
    if (r == RES_EMBER)                  return 8.0f; /* every ember is precious */
    if (threshold <= 1.0)                return 7.0f; /* first-of-a-kind */
    if (threshold >= 1000000.0)          return 7.0f; /* million+ milestones */
    if (threshold >= 100000.0)           return 6.0f;
    return 5.0f;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void milestones_check(GameState *state, ResourceId r) {
    if (!state) return;
    if ((int)r < 0 || r >= RES_COUNT) return;

    const MilestoneDef *def   = &milestone_defs[r];
    double              ever  = state->ever_produced[r];
    uint8_t            *fired = &state->milestones_fired[r];

    for (int i = 0; i < EF_MILESTONE_MAX; ++i) {
        double threshold = def->thresholds[i];
        if (threshold <= 0.0) break;               /* end of list */

        uint8_t bit = (uint8_t)(1u << i);
        if (*fired & bit)   continue;              /* already notified */
        if (ever < threshold) break;               /* haven't reached it yet */

        /* New milestone crossed — fire it */
        *fired |= bit;

        char msg[256];
        milestone_label(threshold, resource_name(r), msg, sizeof(msg));

        float duration = milestone_duration(threshold, r);

        /* Use the resource's own palette colour; fall back to gold for dark
         * resources so the text stays readable against the popup background. */
        PaletteColor color = resource_color(r);
        if (color == PAL_DARK || color == PAL_BG || color == PAL_BG_PANEL)
            color = PAL_GOLD;

        /* Route to the dedicated milestone popup queue (not the generic
         * bottom-bar notif strip, which is easy to miss). */
        milestone_popups_add(state, msg, duration, color);
    }
}
