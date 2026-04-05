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
 * Design intent:
 *   tier 0  (Stone, Coal, Coins)   — you produce these fast; start at 100
 *   tier 1  (Iron/Copper/Tin ore)  — meaningful but plentiful; start at 10
 *   tier 2  (Iron Bar, Copper Bar) — smelted goods; first one is exciting
 *   tier 3  (Bronze, Gear, Bellows, Rivets)
 *   tier 4  (Steel Bar, Steel Plate)
 *   tier 5  (Machine Part, Mythril)
 *   tier 6  (Ember Crystal)        — ultra-rare; every single one counts
 */

#define EF_MILESTONE_MAX 5

typedef struct {
    double thresholds[EF_MILESTONE_MAX]; /* ascending; 0.0 = end of list */
} MilestoneDef;

static const MilestoneDef milestone_defs[RES_COUNT] = {
    /* RES_STONE       tier 0 */ {{   100,  1000, 10000, 100000,      0 }},
    /* RES_COAL        tier 0 */ {{   100,  1000, 10000,      0,      0 }},
    /* RES_COINS       tier 0 */ {{   100,  1000, 10000, 100000,      0 }},
    /* RES_IRON        tier 1 */ {{    10,   100,  1000,      0,      0 }},
    /* RES_COPPER      tier 1 */ {{    10,   100,  1000,      0,      0 }},
    /* RES_TIN         tier 1 */ {{    10,   100,  1000,      0,      0 }},
    /* RES_IRON_BAR    tier 2 */ {{     1,    10,    50,    100,      0 }},
    /* RES_COPPER_BAR  tier 2 */ {{     1,    10,    50,    100,      0 }},
    /* RES_BRONZE      tier 3 */ {{     1,    10,    25,     50,      0 }},
    /* RES_GEAR        tier 3 */ {{     1,    10,    25,     50,      0 }},
    /* RES_BELLOWS     tier 3 */ {{     1,    10,    25,      0,      0 }},
    /* RES_STEEL       tier 4 */ {{     1,     5,    20,     50,      0 }},
    /* RES_STEEL_PLATE tier 4 */ {{     1,     5,    20,     50,      0 }},
    /* RES_RIVET       tier 3 */ {{     1,    25,   100,      0,      0 }},
    /* RES_MACHINE_PART tier 5*/ {{     1,     5,    10,      0,      0 }},
    /* RES_MYTHRIL     tier 5 */ {{     1,     5,    10,      0,      0 }},
    /* RES_EMBER       tier 6 */ {{     1,     3,     5,      0,      0 }},
};

/* Compile-time check: table must cover every ResourceId */
typedef char _milestone_table_size_check[
    (sizeof(milestone_defs) / sizeof(milestone_defs[0]) == RES_COUNT) ? 1 : -1
];

/* ── Internal helpers ───────────────────────────────────────────────────── */

static const char *milestone_label(double threshold, const char *name,
                                   char *buf, size_t buf_len) {
    if (threshold <= 1.0) {
        (void)snprintf(buf, buf_len, "★ First %s!", name);
    } else {
        char num[32];
        fmt_number(num, sizeof(num), threshold);
        (void)snprintf(buf, buf_len, "★ %s %s collected!", num, name);
    }
    return buf;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void milestones_check(GameState *state, ResourceId r) {
    if (!state) return;
    if ((int)r < 0 || r >= RES_COUNT) return;

    const MilestoneDef *def  = &milestone_defs[r];
    double              ever = state->ever_produced[r];
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

        /* Use a longer display time for rarer milestones */
        float duration = (threshold <= 1.0) ? 5.0f : 4.0f;

        /* Color: use the resource's own palette color so each milestone
         * feels distinct; fall back to gold for things that are PAL_DARK
         * (Coal) so the text stays readable. */
        PaletteColor color = resource_color(r);
        if (color == PAL_DARK || color == PAL_BG || color == PAL_BG_PANEL)
            color = PAL_GOLD;

        notifs_add(state, msg, duration, color);
    }
}
