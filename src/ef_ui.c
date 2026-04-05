#include "ef_ui.h"

#include "ef_buildings.h"
#include "ef_events.h"
#include "ef_fonts.h"
#include "ef_format.h"
#include "ef_game.h"
#include "ef_prestige.h"
#include "ef_recipes.h"
#include "ef_resources.h"
#include "ef_save.h"
#include "ef_shop.h"
#include "ef_state.h"
#include "ef_time.h"
#include "ef_upgrades.h"
#include "ef_util.h"

#if __has_include(<SDL2/SDL_image.h>)
#include <SDL2/SDL_image.h>
#else
#include <SDL_image.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum TooltipPlacement {
  TOOLTIP_MOUSE = 0,
  TOOLTIP_ABOVE = 1,
} TooltipPlacement;

typedef enum TextAlign {
  ALIGN_LEFT = 0,
  ALIGN_CENTER = 1,
  ALIGN_RIGHT = 2,
} TextAlign;

typedef struct UiContext {
  int mouse_x;
  int mouse_y;
  bool clicked;
  bool click_used;
  bool input_blocked;
  bool tooltip_set;
  char tooltip_text[1024];
  int tooltip_x;
  int tooltip_y;
  TooltipPlacement tooltip_placement;
  /* inline text input for slot renaming */
  bool text_input_active;
  int  text_input_slot;    /* 1-3: which slot is being renamed */
  char text_input_buf[32];
} UiContext;

static UiContext ui;

/* ── timestamp helper ─────────────────────────────────────────────────────── */

static void format_timestamp(char *out, size_t n, uint64_t ts) {
  if (!ts) { (void)snprintf(out, n, "Empty"); return; }
  time_t t = (time_t)ts;
  struct tm *tm_ptr = localtime(&t);
  if (!tm_ptr) { (void)snprintf(out, n, "?"); return; }
  (void)snprintf(out, n, "%04d-%02d-%02d  %02d:%02d",
                 tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday,
                 tm_ptr->tm_hour, tm_ptr->tm_min);
}

/* ── text-input API ───────────────────────────────────────────────────────── */

void ui_feed_text(const char *text) {
  if (!ui.text_input_active || !text) return;
  size_t cur = strlen(ui.text_input_buf);
  for (const char *p = text; *p && cur < 31; ++p, ++cur)
    ui.text_input_buf[cur] = *p;
  ui.text_input_buf[cur] = '\0';
}

void ui_feed_backspace(void) {
  if (!ui.text_input_active) return;
  size_t n = strlen(ui.text_input_buf);
  if (n > 0) ui.text_input_buf[n - 1] = '\0';
}

bool ui_text_input_active(void) {
  return ui.text_input_active;
}

void ui_commit_text_input(GameState *state) {
  if (!ui.text_input_active || !state) return;
  int s = ui.text_input_slot;
  if (s >= 1 && s <= 3) {
    snprintf(state->slot_names[s - 1], sizeof(state->slot_names[0]), "%s", ui.text_input_buf);
    (void)save_options(state);
  }
  SDL_StopTextInput();
  ui.text_input_active = false;
  ui.text_input_slot   = 0;
  ui.text_input_buf[0] = '\0';
}

void ui_cancel_text_input(void) {
  if (ui.text_input_active) SDL_StopTextInput();
  ui.text_input_active = false;
  ui.text_input_slot   = 0;
  ui.text_input_buf[0] = '\0';
}

void ui_begin_frame(int mouse_x, int mouse_y, bool clicked) {
  ui.mouse_x = mouse_x;
  ui.mouse_y = mouse_y;
  ui.clicked = clicked;
  ui.click_used = false;
  ui.tooltip_set = false;
  ui.tooltip_text[0] = '\0';
  ui.tooltip_x = 0;
  ui.tooltip_y = 0;
  ui.tooltip_placement = TOOLTIP_MOUSE;
}

void ui_set_input_blocked(bool blocked) {
  ui.input_blocked = blocked;
}

static bool point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
  return x >= rx && y >= ry && x < (rx + rw) && y < (ry + rh);
}

static bool rect_hovered(int x, int y, int w, int h) {
  return point_in_rect(ui.mouse_x, ui.mouse_y, x, y, w, h);
}

static bool ui_take_click(void) {
  if (ui.clicked && !ui.click_used) {
    ui.click_used = true;
    return true;
  }
  return false;
}

static void ui_set_tooltip(const char *text, int x, int y, TooltipPlacement placement) {
  if (!text || text[0] == '\0') {
    return;
  }
  ui.tooltip_set = true;
  strncpy(ui.tooltip_text, text, sizeof(ui.tooltip_text) - 1);
  ui.tooltip_text[sizeof(ui.tooltip_text) - 1] = '\0';
  ui.tooltip_x = x;
  ui.tooltip_y = y;
  ui.tooltip_placement = placement;
}

static void set_render_color(SDL_Renderer *renderer, ColorRGBA color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

static void set_render_color_palette(SDL_Renderer *renderer, PaletteColor color) {
  set_render_color(renderer, palette_rgba(color));
}

static void draw_rect(SDL_Renderer *renderer, int x, int y, int w, int h, ColorRGBA fill, bool has_border, ColorRGBA border) {
  SDL_Rect r = {x, y, w, h};
  set_render_color(renderer, fill);
  SDL_RenderFillRect(renderer, &r);
  if (has_border) {
    set_render_color(renderer, border);
    SDL_RenderDrawRect(renderer, &r);
  }
}

static void draw_rect_palette(SDL_Renderer *renderer, int x, int y, int w, int h, PaletteColor fill, bool has_border, PaletteColor border) {
  draw_rect(renderer, x, y, w, h, palette_rgba(fill), has_border, palette_rgba(border));
}

static int text_width(TTF_Font *font, const char *text) {
  if (!font) {
    return 0;
  }
  int w = 0;
  int h = 0;
  if (TTF_SizeUTF8(font, text ? text : "", &w, &h) != 0) {
    return 0;
  }
  return w;
}

static void fit_text_to_width(char *out, size_t out_len, TTF_Font *font, const char *text, int max_width, const char *ellipsis) {
  if (!out || out_len == 0) {
    return;
  }
  out[0] = '\0';
  if (!text) {
    return;
  }

  int mw = max_width > 0 ? max_width : 0;
  if (mw <= 0 || text[0] == '\0') {
    return;
  }

  if (text_width(font, text) <= mw) {
    (void)snprintf(out, out_len, "%s", text);
    return;
  }

  const char *ell = ellipsis && ellipsis[0] != '\0' ? ellipsis : "…";
  int ell_w = text_width(font, ell);
  if (ell_w >= mw) {
    return;
  }

  size_t len = strlen(text);
  size_t low = 0;
  size_t high = len;
  size_t best = 0;

  while (low <= high) {
    size_t mid = (low + high) / 2;
    char candidate[1024];
    size_t mid_clamped = mid < sizeof(candidate) - 1 ? mid : sizeof(candidate) - 1;
    memcpy(candidate, text, mid_clamped);
    candidate[mid_clamped] = '\0';
    strncat(candidate, ell, sizeof(candidate) - strlen(candidate) - 1);

    if (text_width(font, candidate) <= mw) {
      best = mid;
      low = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      high = mid - 1;
    }
  }

  char result[1024];
  size_t best_clamped = best < sizeof(result) - 1 ? best : sizeof(result) - 1;
  memcpy(result, text, best_clamped);
  result[best_clamped] = '\0';
  strncat(result, ell, sizeof(result) - strlen(result) - 1);

  (void)snprintf(out, out_len, "%s", result);
}

static void draw_label(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, PaletteColor color, TextAlign align) {
  if (!font) {
    return;
  }

  ColorRGBA c = palette_rgba(color);
  SDL_Color sc = {c.r, c.g, c.b, c.a};
  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text ? text : "", sc);
  if (!surface) {
    return;
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
  int w = surface->w;
  int h = surface->h;
  SDL_FreeSurface(surface);
  if (!tex) {
    return;
  }

  int tx = x;
  if (align == ALIGN_CENTER) {
    tx = x - (w / 2);
  } else if (align == ALIGN_RIGHT) {
    tx = x - w;
  }

  SDL_Rect dst = {tx, y, w, h};
  SDL_RenderCopy(renderer, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

static void draw_multiline_label(SDL_Renderer *renderer,
                                 TTF_Font *font,
                                 const char *text,
                                 int x,
                                 int y,
                                 PaletteColor color,
                                 int line_height,
                                 TextAlign align) {
  if (!text) {
    return;
  }
  int lh = line_height > 0 ? line_height : 18;

  const char *start = text;
  int line_idx = 0;
  while (start) {
    const char *nl = strchr(start, '\n');
    size_t len = nl ? (size_t)(nl - start) : strlen(start);

    char line[1024];
    size_t copy_len = len < sizeof(line) - 1 ? len : sizeof(line) - 1;
    memcpy(line, start, copy_len);
    line[copy_len] = '\0';
    draw_label(renderer, font, line, x, y + (line_idx * lh), color, align);

    line_idx += 1;
    if (!nl) {
      break;
    }
    start = nl + 1;
  }
}

static void draw_label_fit(SDL_Renderer *renderer,
                           TTF_Font *font,
                           const char *text,
                           int x,
                           int y,
                           PaletteColor color,
                           int max_width,
                           TextAlign align) {
  char fitted[1024];
  fit_text_to_width(fitted, sizeof(fitted), font, text ? text : "", max_width, "…");
  draw_label(renderer, font, fitted, x, y, color, align);
}

static void draw_progress_bar(SDL_Renderer *renderer,
                              int x,
                              int y,
                              int w,
                              int h,
                              float progress,
                              PaletteColor fill,
                              PaletteColor bg) {
  draw_rect_palette(renderer, x, y, w, h, bg, true, PAL_BTN_HOVER);
  float p = progress;
  if (p < 0.0f) {
    p = 0.0f;
  }
  if (p > 1.0f) {
    p = 1.0f;
  }
  int pw = (int)lround((double)w * (double)p);
  if (pw > 0) {
    draw_rect_palette(renderer, x, y, pw, h, fill, false, PAL_BTN_HOVER);
  }
}

static bool draw_button(SDL_Renderer *renderer,
                        TTF_Font *font,
                        const char *text,
                        int x,
                        int y,
                        int w,
                        int h,
                        bool enabled,
                        PaletteColor color,
                        const char *tooltip) {
  bool hovered = rect_hovered(x, y, w, h);
  PaletteColor fill = color;

  if (!enabled) {
    fill = PAL_BG_PANEL;
  } else if (!ui.input_blocked && hovered && ui.clicked) {
    fill = PAL_BTN_ACTIVE;
  } else if (hovered) {
    fill = PAL_BTN_HOVER;
  }

  bool clicked = !ui.input_blocked && enabled && hovered && ui_take_click();

  if (!ui.input_blocked && hovered && tooltip) {
    ui_set_tooltip(tooltip, ui.mouse_x, ui.mouse_y, TOOLTIP_MOUSE);
  }

  draw_rect_palette(renderer, x, y, w, h, fill, true, hovered ? PAL_BTN_ACTIVE : PAL_BTN_HOVER);
  draw_label(renderer, font, text ? text : "", x + 10, y + 6, enabled ? PAL_TEXT : PAL_TEXT_DIM, ALIGN_LEFT);
  return clicked;
}

static void draw_resource_icon(SDL_Renderer *renderer, ResourceId resource, int x, int y) {
  draw_rect_palette(renderer, x, y, 12, 12, resource_color(resource), true, PAL_TEXT_DIM);
}

typedef enum BackgroundId {
  BG_MENU = 0,
  BG_FORGE = 1,
  BG_MINE = 2,
  BG_UPGRADES = 3,
  BG_RECIPES = 4,
  BG_SHOP = 5,
  BG_PRESTIGE = 6,
  BG_COUNT = 7,
} BackgroundId;

static const char *const background_files[BG_COUNT] = {
    [BG_MENU] = "assets/backgrounds/main_menu.png",
    [BG_FORGE] = "assets/backgrounds/forge.png",
    [BG_MINE] = "assets/backgrounds/mine.png",
    [BG_UPGRADES] = "assets/backgrounds/upgrades.png",
    [BG_RECIPES] = "assets/backgrounds/recipes.png",
    [BG_SHOP] = "assets/backgrounds/shop.png",
    [BG_PRESTIGE] = "assets/backgrounds/prestige.png",
};

static SDL_Texture *background_textures[BG_COUNT] = {0};

void clear_panel_backgrounds(void) {
  for (int i = 0; i < BG_COUNT; ++i) {
    if (background_textures[i]) {
      SDL_DestroyTexture(background_textures[i]);
      background_textures[i] = NULL;
    }
  }
}

static SDL_Texture *ensure_background(SDL_Renderer *renderer, BackgroundId bg) {
  if ((int)bg < 0 || bg >= BG_COUNT) {
    return NULL;
  }
  if (background_textures[bg]) {
    return background_textures[bg];
  }
  const char *path = background_files[bg];
  if (!path) {
    return NULL;
  }
  SDL_Surface *surface = IMG_Load(path);
  if (!surface) {
    return NULL;
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
  if (!tex) {
    return NULL;
  }
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  background_textures[bg] = tex;
  return tex;
}

static void draw_panel_background(SDL_Renderer *renderer, BackgroundId bg, int x, int y, int w, int h, ColorRGBA overlay) {
  SDL_Texture *tex = ensure_background(renderer, bg);
  if (tex) {
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    draw_rect(renderer, x, y, w, h, overlay, true, palette_rgba(PAL_BTN_HOVER));
    return;
  }
  draw_rect_palette(renderer, x, y, w, h, PAL_BG_PANEL, true, PAL_BTN_HOVER);
}

static void render_tooltip(SDL_Renderer *renderer, TTF_Font *font) {
  if (!ui.tooltip_set || ui.tooltip_text[0] == '\0' || !font) {
    return;
  }

  /* Maximum pixel width for a single tooltip line.
   * Lines longer than this are word-wrapped so the box never overflows
   * the window, regardless of how many resources appear in a cost list. */
  int padding  = 10;
  int max_line_px = 420;

  /* ── Word-wrap the tooltip text into final_lines[] ── */
  char final_lines[32][512];
  int  final_count = 0;

  const char *seg = ui.tooltip_text;
  while (*seg && final_count < 31) {
    /* Find end of this newline-delimited segment */
    const char *nl = strchr(seg, '\n');
    size_t seg_len = nl ? (size_t)(nl - seg) : strlen(seg);
    char seg_buf[512];
    size_t copy = seg_len < sizeof(seg_buf) - 1 ? seg_len : sizeof(seg_buf) - 1;
    memcpy(seg_buf, seg, copy);
    seg_buf[copy] = '\0';

    /* Check if this segment fits in one line */
    int sw = 0, sh = 0;
    (void)TTF_SizeUTF8(font, seg_buf, &sw, &sh);
    if (sw <= max_line_px) {
      /* Fits — emit as-is */
      (void)snprintf(final_lines[final_count++], 512, "%s", seg_buf);
    } else {
      /* Doesn't fit — word-wrap by spaces */
      char *words[128];
      int  wcount = 0;
      char tmp[512];
      (void)snprintf(tmp, sizeof(tmp), "%s", seg_buf);
      char *tok = strtok(tmp, " ");
      while (tok && wcount < 127) words[wcount++] = tok, tok = strtok(NULL, " ");

      char cur[512] = "";
      for (int wi = 0; wi < wcount && final_count < 31; ++wi) {
        char candidate[512];
        if (cur[0]) {
          (void)snprintf(candidate, sizeof(candidate), "%s %s", cur, words[wi]);
        } else {
          (void)snprintf(candidate, sizeof(candidate), "%s", words[wi]);
        }
        int cw = 0, ch = 0;
        (void)TTF_SizeUTF8(font, candidate, &cw, &ch);
        if (cw <= max_line_px) {
          (void)snprintf(cur, sizeof(cur), "%s", candidate);
        } else {
          if (cur[0]) (void)snprintf(final_lines[final_count++], 512, "%s", cur);
          (void)snprintf(cur, sizeof(cur), "%s", words[wi]);
        }
      }
      if (cur[0] && final_count < 31) (void)snprintf(final_lines[final_count++], 512, "%s", cur);
    }

    seg = nl ? nl + 1 : seg + seg_len;
    if (!nl) break;
  }

  /* ── Measure the wrapped lines to size the box ── */
  int max_w = 0;
  int max_h = 0;
  for (int li = 0; li < final_count; ++li) {
    int w = 0, h = 0;
    (void)TTF_SizeUTF8(font, final_lines[li], &w, &h);
    if (w > max_w) max_w = w;
    if (h > max_h) max_h = h;
  }

  int line_h = max_h + 2;
  if (line_h < 14) {
    line_h = 14;
  }

  int box_w = max_w + (2 * padding);
  int box_h = (final_count * line_h) + (2 * padding);

  int anchor_x = ui.tooltip_x;
  int anchor_y = ui.tooltip_y;

  int x = 0;
  int y = 0;
  if (ui.tooltip_placement == TOOLTIP_MOUSE) {
    x = anchor_x + 16;
    y = anchor_y + 16;
    if (x + box_w > WINDOW_WIDTH) {
      x = anchor_x - box_w - 16;
      if (x < 0) {
        x = 0;
      }
    }
    if (y + box_h > WINDOW_HEIGHT) {
      y = anchor_y - box_h - 16;
      if (y < 0) {
        y = 0;
      }
    }
  } else {
    x = anchor_x;
    y = anchor_y - box_h - 12;
    if (y < 0) {
      y = anchor_y + 16;
    }
    if (x + box_w > WINDOW_WIDTH) {
      x = WINDOW_WIDTH - box_w;
      if (x < 0) {
        x = 0;
      }
    }
    if (y + box_h > WINDOW_HEIGHT) {
      y = WINDOW_HEIGHT - box_h;
      if (y < 0) {
        y = 0;
      }
    }
  }

  draw_rect(renderer, x, y, box_w, box_h, (ColorRGBA){0, 0, 0, 220}, true, palette_rgba(PAL_BTN_HOVER));
  for (int li = 0; li < final_count; ++li) {
    draw_label(renderer, font, final_lines[li],
               x + padding, y + padding + li * line_h, PAL_TEXT, ALIGN_LEFT);
  }
}

static void render_notifications(SDL_Renderer *renderer, TTF_Font *font, const GameState *state, int x, int y, int w, int h) {
  draw_rect_palette(renderer, x, y, w, h, PAL_BG_TOP, true, PAL_BTN_HOVER);
  if (!state || state->notifications.len == 0) {
    return;
  }
  const Notif *msg = &state->notifications.items[0];
  draw_label(renderer, font, msg->text, x + 10, y + 6, msg->color, ALIGN_LEFT);
}

static void render_top_bar(SDL_Renderer *renderer, const GameState *state, TTF_Font *font) {
  draw_rect_palette(renderer, 0, 0, WINDOW_WIDTH, 32, PAL_BG_TOP, true, PAL_BTN_HOVER);
  draw_label(renderer, font, "Ember Forge", 12, 7, PAL_TEXT, ALIGN_LEFT);

  char coins_buf[64];
  char stone_buf[64];
  char coal_buf[64];
  fmt_number(coins_buf, sizeof(coins_buf), resource_amount(state, RES_COINS));
  fmt_number(stone_buf, sizeof(stone_buf), resource_amount(state, RES_STONE));
  fmt_number(coal_buf, sizeof(coal_buf), resource_amount(state, RES_COAL));

  char line[128];
  (void)snprintf(line, sizeof(line), "Coins: %s", coins_buf);
  draw_label(renderer, font, line, 220, 7, PAL_GOLD, ALIGN_LEFT);

  (void)snprintf(line, sizeof(line), "Stone: %s", stone_buf);
  draw_label(renderer, font, line, 400, 7, PAL_TEXT_DIM, ALIGN_LEFT);

  (void)snprintf(line, sizeof(line), "Coal: %s", coal_buf);
  draw_label(renderer, font, line, 560, 7, PAL_TEXT_DIM, ALIGN_LEFT);

  (void)snprintf(line, sizeof(line), "FPS: %d", state ? state->fps : 0);
  draw_label(renderer, font, line, WINDOW_WIDTH - 80, 7, PAL_TEXT_DIM, ALIGN_LEFT);
}

static void render_nav_tabs(SDL_Renderer *renderer, GameState *state, TTF_Font *font) {
  draw_rect_palette(renderer, 0, 32, WINDOW_WIDTH, 32, PAL_BG_PANEL, true, PAL_BTN_HOVER);

  struct Tab {
    Panel id;
    const char *label;
  };
  static const struct Tab tabs[] = {
      {PANEL_FORGE, "Forge"}, {PANEL_MINE, "Mine"}, {PANEL_UPGRADES, "Upgrades"}, {PANEL_RECIPES, "Recipes"}, {PANEL_SHOP, "Shop"}, {PANEL_PRESTIGE, "Prestige"},
  };

  int x = 12;
  int y = 36;
  for (size_t i = 0; i < EF_ARRAY_COUNT(tabs); ++i) {
    bool active = state->active_panel == tabs[i].id;
    if (draw_button(renderer, font, tabs[i].label, x, y, 120, 24, true, active ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL, NULL)) {
      state->active_panel = tabs[i].id;
    }
    x += 120 + 8;
  }
}

static int compare_resource_rows(const void *a, const void *b) {
  const ResourceId *ra = (const ResourceId *)a;
  const ResourceId *rb = (const ResourceId *)b;
  int ta = resource_tier(*ra);
  int tb = resource_tier(*rb);
  if (ta != tb) {
    return ta < tb ? -1 : 1;
  }
  return strcmp(resource_name(*ra), resource_name(*rb));
}

static size_t build_unlocked_resource_list(const GameState *state, ResourceId *out, size_t out_cap) {
  size_t n = 0;
  for (int i = 0; i < RES_COUNT && n < out_cap; ++i) {
    if (resource_unlocked_p(state, (ResourceId)i)) {
      out[n++] = (ResourceId)i;
    }
  }
  qsort(out, n, sizeof(out[0]), compare_resource_rows);
  return n;
}

static void render_resource_sidebar(SDL_Renderer *renderer, const GameState *state, TTF_Font *font, int x, int y, int w, int h) {
  draw_rect_palette(renderer, x, y, w, h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);
  draw_label(renderer, font, "RESOURCES", x + 14, y + 10, PAL_TEXT, ALIGN_LEFT);

  ResourceId rows[RES_COUNT];
  size_t row_count = build_unlocked_resource_list(state, rows, EF_ARRAY_COUNT(rows));

  int row_h = 26;
  int y0 = y + 36;

  for (size_t i = 0; i < row_count; ++i) {
    int yy = y0 + (int)i * (row_h + 4);
    if (yy + row_h >= y + h - 8) {
      break;
    }
    ResourceId res = rows[i];
    double amt = resource_amount(state, res);
    double cap = resource_cap((GameState *)state, res);
    double rate = state->production[res];

    draw_resource_icon(renderer, res, x + 14, yy + 7);
    draw_label(renderer, font, resource_name(res), x + 32, yy + 4, PAL_TEXT, ALIGN_LEFT);

    char amt_buf[64];
    char cap_buf[64];
    fmt_number(amt_buf, sizeof(amt_buf), amt);
    fmt_number(cap_buf, sizeof(cap_buf), cap);

    draw_label(renderer, font, amt_buf, x + 160, yy + 4, PAL_TEXT_DIM, ALIGN_LEFT);

    char cap_label[80];
    (void)snprintf(cap_label, sizeof(cap_label), "/%s", cap_buf);
    draw_label(renderer, font, cap_label, x + 220, yy + 4, PAL_TEXT_DIM, ALIGN_LEFT);

    if (rate != 0.0) {
      char rate_buf[64];
      fmt_rate(rate_buf, sizeof(rate_buf), rate);
      draw_label(renderer, font, rate_buf, x + 32, yy + 18, PAL_TEXT_DIM, ALIGN_LEFT);
    }
  }
}

static void fmt_cost(char *out, size_t out_len, const ResAmount *items, size_t item_count) {
  if (!out || out_len == 0) {
    return;
  }
  out[0] = '\0';
  for (size_t i = 0; i < item_count; ++i) {
    char amt[64];
    fmt_number(amt, sizeof(amt), items[i].amount);
    char chunk[128];
    (void)snprintf(chunk, sizeof(chunk), "%s %s", amt, resource_name(items[i].res));

    if (i > 0) {
      strncat(out, ", ", out_len - strlen(out) - 1);
    }
    strncat(out, chunk, out_len - strlen(out) - 1);
  }
}

static void fmt_io(char *out, size_t out_len, const ResAmount *items, size_t item_count) {
  fmt_cost(out, out_len, items, item_count);
}

static void render_panel_forge(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  draw_panel_background(renderer, BG_FORGE, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "FORGE", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  int btn_w = 360;
  int btn_h = 44;
  int btn_x = x + (w - btn_w) / 2;
  int btn_y = y + 48;

  char label[128];
  char tooltip[128];

  if (state->manual_mine.active) {
    (void)snprintf(label, sizeof(label), "MINING... %.1Fs", state->manual_mine.timer);
    (void)snprintf(tooltip, sizeof(tooltip), "Mining... %.1Fs remaining.", state->manual_mine.timer);
  } else {
    (void)snprintf(label, sizeof(label), "CLICK TO MINE");
    (void)snprintf(tooltip, sizeof(tooltip), "Mine stone (takes %.1Fs).", state->manual_mine_duration);
  }

  if (draw_button(renderer, font, label, btn_x, btn_y, btn_w, btn_h, !state->manual_mine.active, PAL_BTN_NORMAL, tooltip)) {
    (void)start_manual_mine(state);
  }

  if (state->manual_mine.active) {
    draw_progress_bar(renderer, btn_x, btn_y + btn_h + 6, btn_w, 10, manual_job_progress(&state->manual_mine), PAL_GOLD, PAL_BG_SIDEBAR);
  }

  char click_buf[64];
  fmt_number(click_buf, sizeof(click_buf), state->click_mult);
  char click_line[128];
  (void)snprintf(click_line, sizeof(click_line), "Click power: %s stone", click_buf);
  draw_label(renderer, font, click_line, x + 24, y + 104, PAL_TEXT_DIM, ALIGN_LEFT);

  int q_x = x + 24;
  int q_y = y + 152;  /* pushed down so "Click power" and "SMELTING QUEUE" don't overlap */
  int col_gap = 24;
  int half_w  = (w - 48 - col_gap) / 2;
  int q_w     = half_w;
  int row_h   = 28;
  int max_slots = max_smelt_slots(state);
  int shown = max_slots < 6 ? max_slots : 6;

  draw_label(renderer, font, "SMELTING QUEUE", q_x, q_y - 18, PAL_TEXT, ALIGN_LEFT);
  for (int i = 0; i < shown; ++i) {
    int row_y = q_y + i * (row_h + 6);
    draw_rect_palette(renderer, q_x, row_y, q_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);

    if ((size_t)i < state->smelt_queue.len) {
      SmeltJob job = state->smelt_queue.items[i];
      const RecipeDef *def = recipe_def(job.recipe);
      const char *name = def ? def->name : "???";

      draw_label(renderer, font, name, q_x + 10, row_y + 6, PAL_TEXT, ALIGN_LEFT);
      int pb_x = q_x + q_w - 170;
      draw_progress_bar(renderer, pb_x, row_y + 8, 80, 12, job.progress, PAL_GOLD, PAL_BG_PANEL);

      if (draw_button(renderer, font, "Cancel", q_x + q_w - 80, row_y + 2, 72, row_h - 4, true, PAL_BTN_NORMAL, NULL)) {
        (void)cancel_smelt_job(state, (size_t)i);
      }
    } else {
      draw_label(renderer, font, (i < max_slots) ? "(empty)" : "(locked)", q_x + 10, row_y + 6, PAL_TEXT_DIM, ALIGN_LEFT);
    }
  }

  /* ── Dedicated Crucibles column (right half) ──────────────────────────── */
  int c_x   = q_x + half_w + col_gap;
  int c_w   = half_w;
  int c_y   = q_y;
  int c_row = 28;

  draw_label(renderer, font, "DEDICATED CRUCIBLES", c_x, c_y - 18, PAL_TEXT, ALIGN_LEFT);

  if (state->crucibles_purchased == 0) {
    draw_label(renderer, font, "Buy in the Shop to unlock.", c_x + 10, c_y + 8, PAL_TEXT_DIM, ALIGN_LEFT);
  }

  /* Build list of available recipes for cycling */
  RecipeId avail[RCP_COUNT];
  int avail_count = 0;
  for (int id = 0; id < RCP_COUNT; ++id) {
    const RecipeDef *def = recipe_def((RecipeId)id);
    if (def && recipe_available(state, def))
      avail[avail_count++] = (RecipeId)id;
  }

  for (int i = 0; i < state->crucibles_purchased && i < MAX_CRUCIBLES; ++i) {
    CrucibleSlot *slot = &state->crucibles[i];
    int row_y    = c_y + i * (c_row + 10);
    int row_full = c_row + 10;

    draw_rect_palette(renderer, c_x, row_y, c_w, row_full, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);

    /* ← / → cycle recipe buttons */
    int arrow_w = 22;
    int arrow_h = row_full;
    if (draw_button(renderer, font, "<", c_x, row_y, arrow_w, arrow_h, avail_count > 0, PAL_BTN_NORMAL, "Previous recipe")) {
      if (avail_count > 0) {
        int cur = 0;
        if (slot->has_recipe)
          for (int k = 0; k < avail_count; ++k)
            if (avail[k] == slot->recipe) { cur = k; break; }
        crucible_set_recipe(state, i, avail[(cur - 1 + avail_count) % avail_count]);
        crucible_start(state, i);
      }
    }
    if (draw_button(renderer, font, ">", c_x + arrow_w, row_y, arrow_w, arrow_h, avail_count > 0, PAL_BTN_NORMAL, "Next recipe")) {
      if (avail_count > 0) {
        int cur = 0;
        if (slot->has_recipe)
          for (int k = 0; k < avail_count; ++k)
            if (avail[k] == slot->recipe) { cur = k; break; }
        crucible_set_recipe(state, i, avail[(cur + 1) % avail_count]);
        crucible_start(state, i);
      }
    }

    /* Clear button on the far right */
    int clear_w = 46;
    int clear_x = c_x + c_w - clear_w;
    if (draw_button(renderer, font, "Clear", clear_x, row_y, clear_w, arrow_h, slot->has_recipe, PAL_BTN_NORMAL, "Stop and clear this crucible")) {
      crucible_stop(state, i);
    }

    /* Name + progress in the middle */
    int inner_x = c_x + arrow_w * 2 + 6;
    int inner_w = clear_x - inner_x - 6;
    if (!slot->has_recipe) {
      draw_label_fit(renderer, font, "(use < > to assign recipe)", inner_x, row_y + 10, PAL_TEXT_DIM, inner_w, ALIGN_LEFT);
    } else {
      PaletteColor nc = slot->stalled ? PAL_WARN : PAL_TEXT;
      draw_label_fit(renderer, font, recipe_name(slot->recipe), inner_x, row_y + 2, nc, inner_w, ALIGN_LEFT);
      if (slot->stalled) {
        draw_label_fit(renderer, font, "Waiting for resources...", inner_x, row_y + 20, PAL_TEXT_DIM, inner_w, ALIGN_LEFT);
      } else {
        draw_progress_bar(renderer, inner_x, row_y + 22, inner_w, 7, slot->progress, PAL_GOLD, PAL_BG_PANEL);
      }
    }
  }

  int b_x = x + 24;
  int b_y = y + 340;
  int b_w = w - 48;
  int building_row_h = 34;
  int shown_rows = 0;

  draw_label(renderer, font, "BUILDINGS", b_x, b_y - 18, PAL_TEXT, ALIGN_LEFT);
  for (int id = 0; id < BLD_COUNT; ++id) {
    const BuildingDef *def = building_def((BuildingId)id);
    if (!def || !building_unlocked(state, def)) {
      continue;
    }
    int row_y = b_y + shown_rows * (building_row_h + 6);
    if (row_y + building_row_h >= y + h - 8) {
      break;
    }
    shown_rows += 1;

    bool hovered = rect_hovered(b_x, row_y, b_w, building_row_h);
    draw_rect_palette(renderer, b_x, row_y, b_w, building_row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);

    int count = building_count(state, def->id);
    char title[128];
    (void)snprintf(title, sizeof(title), "%s  ×%d", def->name, count);
    draw_label(renderer, font, title, b_x + 10, row_y + 6, PAL_TEXT, ALIGN_LEFT);

    ResAmount cost_items[8];
    size_t cost_count = building_cost(def->id, count, cost_items, EF_ARRAY_COUNT(cost_items));
    bool can_buy = can_afford_list(state, cost_items, cost_count);

    char cost_text[256];
    fmt_cost(cost_text, sizeof(cost_text), cost_items, cost_count);

    int cost_right = b_x + (b_w - 96);
    int cost_left = cost_right - text_width(font, cost_text);

    char summary[512];
    if (def->production_count > 0) {
      summary[0] = '\0';
      for (size_t j = 0; j < def->production_count; ++j) {
        double rate = def->production[j].amount * building_prod_mult(state, def->id) * state->prod_mult;
        char rate_buf[64];
        fmt_rate(rate_buf, sizeof(rate_buf), rate);
        char chunk[128];
        (void)snprintf(chunk, sizeof(chunk), "+%s %s", rate_buf, resource_name(def->production[j].res));
        if (j > 0) {
          strncat(summary, ", ", sizeof(summary) - strlen(summary) - 1);
        }
        strncat(summary, chunk, sizeof(summary) - strlen(summary) - 1);
      }
    } else {
      strncpy(summary, def->description, sizeof(summary) - 1);
      summary[sizeof(summary) - 1] = '\0';
    }

    int sum_x = b_x + 320;
    int sum_max_w = cost_left - 12 - sum_x;
    if (sum_max_w < 0) {
      sum_max_w = 0;
    }

    if (hovered) {
      ui_set_tooltip(def->description, b_x + 8, row_y, TOOLTIP_ABOVE);
    }

    draw_label_fit(renderer, font, summary, sum_x, row_y + 6, PAL_TEXT_DIM, sum_max_w, ALIGN_LEFT);
    draw_label(renderer, font, cost_text, cost_right, row_y + 6, PAL_TEXT_DIM, ALIGN_RIGHT);

    if (draw_button(renderer, font, "Buy", b_x + b_w - 86, row_y + 4, 78, building_row_h - 8, can_buy, PAL_BTN_NORMAL, NULL)) {
      (void)buy_building(state, def->id);
    }
  }
}

static void render_panel_mine(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  (void)font_large;
  draw_panel_background(renderer, BG_MINE, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "MINE", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  ResourceId rows[RES_COUNT];
  size_t row_count = build_unlocked_resource_list(state, rows, EF_ARRAY_COUNT(rows));

  int row_h = 34;
  int r_x = x + 24;
  int r_y = y + 56;
  int r_w = w - 48;

  for (size_t i = 0; i < row_count; ++i) {
    int y0 = r_y + (int)i * (row_h + 6);
    if (y0 + row_h >= y + h - 8) {
      break;
    }
    ResourceId res = rows[i];
    double amt = resource_amount(state, res);
    double cap = resource_cap(state, res);
    double rate = state->production[res];

    draw_rect_palette(renderer, r_x, y0, r_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);
    draw_resource_icon(renderer, res, r_x + 10, y0 + 10);
    draw_label(renderer, font, resource_name(res), r_x + 30, y0 + 6, PAL_TEXT, ALIGN_LEFT);

    char amt_buf[64];
    char cap_buf[64];
    fmt_number(amt_buf, sizeof(amt_buf), amt);
    fmt_number(cap_buf, sizeof(cap_buf), cap);
    char amount_line[192];
    (void)snprintf(amount_line, sizeof(amount_line), "%s / %s", amt_buf, cap_buf);
    draw_label(renderer, font, amount_line, r_x + 260, y0 + 6, PAL_TEXT_DIM, ALIGN_LEFT);

    char rate_buf[64];
    fmt_rate(rate_buf, sizeof(rate_buf), rate);
    draw_label(renderer, font, rate_buf, r_x + 460, y0 + 6, PAL_TEXT_DIM, ALIGN_LEFT);

    if (res != RES_COINS && sell_value(res) > 0.0) {
      double value_coins = 10.0 * sell_value(res) * state->sell_mult;
      char val_buf[64];
      fmt_number(val_buf, sizeof(val_buf), value_coins);
      char tooltip[192];
      (void)snprintf(tooltip, sizeof(tooltip), "Sell 10 %s (value: %s coins).", resource_name(res), val_buf);

      if (draw_button(renderer, font, "Sell 10", r_x + (r_w - 280), y0 + 4, 120, row_h - 8, can_sell(state, res, 10.0), PAL_BTN_NORMAL, tooltip)) {
        (void)sell_resource(state, res, 10.0);
      }
    }

    if (resource_tier(res) == 0) {
      bool this_job = state->manual_gather.active && state->manual_gather.resource == res;
      bool enabled = !state->manual_gather.active;

      char label[64];
      if (this_job) {
        (void)snprintf(label, sizeof(label), "Gathering %.1Fs", state->manual_gather.timer);
      } else if (state->manual_gather.active) {
        (void)snprintf(label, sizeof(label), "Busy");
      } else {
        (void)snprintf(label, sizeof(label), "Gather");
      }

      char tooltip[128];
      float g_base = resource_base_gather_time(res);
      float g_mult = (state->gather_mult > 0.1) ? (float)state->gather_mult : 0.1f;
      float g_time = (g_base > 0.0f) ? (g_base / g_mult) : state->manual_gather_duration;
      if (g_time < 0.2f) g_time = 0.2f;
      (void)snprintf(tooltip, sizeof(tooltip), "Gather %s (takes %.1Fs).", resource_name(res), g_time);

      if (draw_button(renderer, font, label, r_x + (r_w - 140), y0 + 4, 132, row_h - 8, enabled, PAL_BTN_NORMAL, tooltip)) {
        (void)start_manual_gather(state, res);
      }

      if (this_job) {
        draw_progress_bar(renderer, r_x + (r_w - 470), y0 + 10, 168, 12, manual_job_progress(&state->manual_gather), PAL_GOLD, PAL_BG_PANEL);
      }
    }
  }
}

static void render_panel_upgrades(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  (void)font_large;
  draw_panel_background(renderer, BG_UPGRADES, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "UPGRADES", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  /* Three-line rows: name / description / cost.
   * Taller rows remove the squeeze that caused cost and description to
   * compete for the same line and truncate each other. */
  int row_h  = 58;
  int row_gap = 6;
  int u_x = x + 24;
  int u_y = y + 56;
  int u_w = w - 48;
  int col_gap = 16;
  int col_w = (u_w - col_gap) / 2;
  if (col_w < 0) {
    col_w = 0;
  }

  int idx = 0;
  for (int id = 0; id < UPG_COUNT; ++id) {
    const UpgradeDef *def = upgrade_def((UpgradeId)id);
    if (!def) {
      continue;
    }
    bool owned = upgrade_owned(state, def->id);
    if (!owned && !upgrade_available(state, def)) {
      continue;
    }

    int col   = idx % 2;
    int row   = idx / 2;
    int col_x = u_x + col * (col_w + col_gap);
    int row_y = u_y + row * (row_h + row_gap);
    idx += 1;
    if (row_y + row_h >= y + h - 8) {
      break;
    }

    bool can_buy = !owned && can_afford_list(state, def->cost, def->cost_count);

    /* Layout: [text area] [Buy button] */
    int btn_w    = 84;
    int btn_h    = row_h - 14;
    int btn_x    = col_x + col_w - btn_w - 8;
    int btn_y    = row_y + 7;
    int text_x   = col_x + 10;
    int text_right = btn_x - 10;
    int text_w   = text_right - text_x;
    if (text_w < 0) text_w = 0;

    /* Build strings */
    char cost_full[256];
    fmt_cost(cost_full, sizeof(cost_full), def->cost, def->cost_count);

    char name_full[256];
    (void)snprintf(name_full, sizeof(name_full), owned ? "\u2713 %s" : "%s", def->name);

    /* Fit each line independently to the full text area width.
     * Cost gets its own line so it never squeezes the description. */
    char name_fit[256];
    char desc_fit[256];
    char cost_fit[256];
    fit_text_to_width(name_fit, sizeof(name_fit), font, name_full,       text_w, "\u2026");
    fit_text_to_width(desc_fit, sizeof(desc_fit), font, def->description, text_w, "\u2026");
    fit_text_to_width(cost_fit, sizeof(cost_fit), font, cost_full,        text_w, "\u2026");

    bool name_cut = strcmp(name_fit, name_full) != 0;
    bool desc_cut = strcmp(desc_fit, def->description) != 0;
    bool cost_cut = strcmp(cost_fit, cost_full) != 0;
    bool hovered  = rect_hovered(col_x, row_y, col_w, row_h);

    /* Row background */
    draw_rect_palette(renderer, col_x, row_y, col_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);

    /* Line 1 — name */
    draw_label(renderer, font, name_fit, text_x, row_y + 5,
               owned ? PAL_TEXT_DIM : PAL_TEXT, ALIGN_LEFT);
    /* Line 2 — description */
    draw_label(renderer, font, desc_fit, text_x, row_y + 22, PAL_TEXT_DIM, ALIGN_LEFT);
    /* Line 3 — cost (gold tint so it reads as a price, not plain text) */
    draw_label(renderer, font, cost_fit, text_x, row_y + 39,
               owned ? PAL_TEXT_DIM : (can_buy ? PAL_GOLD : PAL_RUST), ALIGN_LEFT);

    /* Tooltip on hover — always shown, always includes full cost */
    if (hovered) {
      /* Show full detail whenever any field is cut, or unconditionally so
       * the player can always read the complete cost list on hover. */
      (void)name_cut; (void)desc_cut; (void)cost_cut;
      char tooltip[768];
      (void)snprintf(tooltip, sizeof(tooltip), "%s\n%s\nCost: %s",
                     def->name, def->description, cost_full);
      ui_set_tooltip(tooltip, col_x + 8, row_y, TOOLTIP_ABOVE);
    }

    if (draw_button(renderer, font, owned ? "Owned" : "Buy",
                    btn_x, btn_y, btn_w, btn_h, can_buy, PAL_BTN_NORMAL, NULL)) {
      (void)buy_upgrade(state, def->id);
    }
  }
}

static void render_panel_recipes(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  (void)font_large;
  draw_panel_background(renderer, BG_RECIPES, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "RECIPES", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  int row_h = 66;
  int r_x = x + 24;
  int r_y = y + 56;
  int r_w = w - 48;
  int col_gap = 16;
  int col_w = (r_w - col_gap) / 2;
  bool queue_room = (int)state->smelt_queue.len < max_smelt_slots(state);

  int idx = 0;
  for (int id = 0; id < RCP_COUNT; ++id) {
    const RecipeDef *def = recipe_def((RecipeId)id);
    if (!def || !recipe_available(state, def)) {
      continue;
    }

    int col = idx % 2;
    int row = idx / 2;
    int col_x = r_x + col * (col_w + col_gap);
    int row_y = r_y + row * (row_h + 10);
    idx += 1;
    if (row_y + row_h >= y + h - 8) {
      break;
    }

    bool can_buy = queue_room && can_afford_recipe(state, def);
    int btn_x = col_x + col_w - 92;
    int btn_y = row_y + 10;
    int btn_w = 84;
    int btn_h = row_h - 20;
    int text_x = col_x + 10;
    int text_right = btn_x - 12;
    int text_w = text_right - text_x;
    if (text_w < 0) {
      text_w = 0;
    }

    char in_buf[256];
    char out_buf[256];
    fmt_io(in_buf, sizeof(in_buf), def->inputs, def->input_count);
    fmt_io(out_buf, sizeof(out_buf), def->outputs, def->output_count);

    char in_line[300];
    char out_line[300];
    (void)snprintf(in_line, sizeof(in_line), "In: %s", in_buf);
    (void)snprintf(out_line, sizeof(out_line), "Out: %s", out_buf);

    draw_rect_palette(renderer, col_x, row_y, col_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);
    draw_label_fit(renderer, font, def->name, text_x, row_y + 6, PAL_TEXT, text_w - 70, ALIGN_LEFT);
    draw_label_fit(renderer, font, in_line, text_x, row_y + 24, PAL_TEXT_DIM, text_w, ALIGN_LEFT);
    draw_label_fit(renderer, font, out_line, text_x, row_y + 40, PAL_TEXT_DIM, text_w, ALIGN_LEFT);

    char dur[32];
    (void)snprintf(dur, sizeof(dur), "%.1Fs", def->duration_seconds);
    draw_label(renderer, font, dur, text_right, row_y + 6, PAL_TEXT_DIM, ALIGN_RIGHT);

    if (draw_button(renderer, font, "Smelt", btn_x, btn_y, btn_w, btn_h, can_buy, PAL_BTN_NORMAL, "Queue this recipe into the smelting queue.")) {
      (void)enqueue_smelt_job(state, def->id, SMELT_SOURCE_MANUAL);
    }

    /* Pin button — assign to the first idle or empty dedicated crucible */
    if (state->crucibles_purchased > 0) {
      int pin_w = 46;
      int pin_x = btn_x - pin_w - 6;
      int pin_y = btn_y;
      int pin_h = btn_h;

      /* find a free or stopped crucible slot */
      int free_slot = -1;
      for (int ci = 0; ci < state->crucibles_purchased && ci < MAX_CRUCIBLES; ++ci) {
        if (!state->crucibles[ci].has_recipe) { free_slot = ci; break; }
      }
      /* if all occupied, find one with a different recipe to swap */
      if (free_slot < 0) free_slot = 0; /* default: overwrite slot 0 */

      bool pin_can = (state->crucibles_purchased > 0);
      char pin_tip[128];
      if (free_slot >= 0) {
        (void)snprintf(pin_tip, sizeof(pin_tip), "Pin this recipe to Crucible #%d (auto-repeat).", free_slot + 1);
      } else {
        (void)snprintf(pin_tip, sizeof(pin_tip), "Pin to a dedicated crucible slot.");
      }

      if (draw_button(renderer, font, "Pin", pin_x, pin_y, pin_w, pin_h, pin_can, PAL_BTN_NORMAL, pin_tip)) {
        crucible_set_recipe(state, free_slot, def->id);
        crucible_start(state, free_slot);
        char msg[128];
        (void)snprintf(msg, sizeof(msg), "Pinned %s to Crucible #%d", def->name, free_slot + 1);
        notifs_add(state, msg, 3.0f, PAL_GOLD);
      }
    }
  }
}

static double shop_trade_qty = 10.0;

static bool shop_resource_available(const GameState *state, ResourceId res) {
  return state && res != RES_COINS && sell_value(res) > 0.0 && (resource_unlocked_p(state, res) || resource_tier(res) <= 1);
}

static size_t build_shop_resource_list(const GameState *state, ResourceId *out, size_t out_cap) {
  size_t n = 0;
  for (int i = 0; i < RES_COUNT && n < out_cap; ++i) {
    ResourceId res = (ResourceId)i;
    if (shop_resource_available(state, res)) {
      out[n++] = res;
    }
  }
  qsort(out, n, sizeof(out[0]), compare_resource_rows);
  return n;
}

static void render_panel_shop(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  (void)font_large;
  draw_panel_background(renderer, BG_SHOP, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "SHOP", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  char status[256];
  (void)snprintf(status,
                 sizeof(status),
                 "Sell bonus: ×%.2f    Prices: ×%.2f    Buy mult: ×%.2f",
                 state->sell_mult,
                 state->shop_discount,
                 shop_buy_mult(state));
  draw_label(renderer, font, status, x + 24, y + 52, PAL_TEXT_DIM, ALIGN_LEFT);

  int sx = x + 24;
  int sy = y + 76;
  draw_label(renderer, font, "Trade:", sx, sy + 6, PAL_TEXT_DIM, ALIGN_LEFT);

  int bx = sx + 64;
  int btn_w = 74;
  int btn_h = 26;
  int gap = 8;
  double quantities[] = {1.0, 10.0, 100.0};
  for (size_t i = 0; i < EF_ARRAY_COUNT(quantities); ++i) {
    bool active = fabs(shop_trade_qty - quantities[i]) < 0.001;
    char label[16];
    (void)snprintf(label, sizeof(label), "%d", (int)quantities[i]);
    if (draw_button(renderer, font, label, bx, sy, btn_w, btn_h, true, active ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL, NULL)) {
      shop_trade_qty = quantities[i];
    }
    bx += btn_w + gap;
  }

  int market_x = x + 24;
  int market_y = y + 116;
  int market_w = (int)floor((double)w * 0.58);
  int market_h = h - 140;

  int perks_x = market_x + market_w + 18;
  int perks_y = market_y;
  int perks_w = w - (perks_x - x) - 24;

  draw_label(renderer, font, "MARKET", market_x, market_y - 18, PAL_TEXT, ALIGN_LEFT);

  ResourceId res_list[RES_COUNT];
  size_t res_count = build_shop_resource_list(state, res_list, EF_ARRAY_COUNT(res_list));

  int row_h = 34;
  int btn_trade_w = 70;
  int btn_trade_h = row_h - 10;
  double buy_mult = shop_buy_mult(state);

  for (size_t i = 0; i < res_count; ++i) {
    int row_y = market_y + (int)i * (row_h + 6);
    if (row_y + row_h >= market_y + market_h) {
      break;
    }

    ResourceId res = res_list[i];
    double amt = resource_amount(state, res);
    double cap = resource_cap(state, res);
    double sell = sell_value(res);

    double qty = shop_trade_qty;
    double sell_coins = sell * qty * state->sell_mult;
    double buy_coins = sell * qty * buy_mult * state->shop_discount;

    bool can_sell_q = can_sell(state, res, qty);
    bool can_buy_q = shop_can_buy_resource(state, res, qty);

    int row_x = market_x;
    int row_w = market_w;
    int btn_buy_x = row_x + row_w - btn_trade_w - 8;
    int btn_sell_x = row_x + row_w - 2 * (btn_trade_w + 8);
    int text_right = btn_sell_x - 10;

    draw_rect_palette(renderer, row_x, row_y, row_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);
    draw_resource_icon(renderer, res, row_x + 10, row_y + 11);
    draw_label(renderer, font, resource_name(res), row_x + 30, row_y + 6, PAL_TEXT, ALIGN_LEFT);

    char amt_buf[64];
    char cap_buf[64];
    fmt_number(amt_buf, sizeof(amt_buf), amt);
    fmt_number(cap_buf, sizeof(cap_buf), cap);
    char amount_line[192];
    (void)snprintf(amount_line, sizeof(amount_line), "%s / %s", amt_buf, cap_buf);
    draw_label(renderer, font, amount_line, row_x + 220, row_y + 6, PAL_TEXT_DIM, ALIGN_LEFT);

    char sell_buf[64];
    char buy_buf[64];
    fmt_number(sell_buf, sizeof(sell_buf), sell_coins);
    fmt_number(buy_buf, sizeof(buy_buf), buy_coins);
    char sell_line[96];
    char buy_line[96];
    (void)snprintf(sell_line, sizeof(sell_line), "Sell: +%s c", sell_buf);
    (void)snprintf(buy_line, sizeof(buy_line), "Buy: -%s c", buy_buf);
    draw_label(renderer, font, sell_line, text_right, row_y + 6, PAL_TEXT_DIM, ALIGN_RIGHT);
    draw_label(renderer, font, buy_line, text_right, row_y + 20, PAL_TEXT_DIM, ALIGN_RIGHT);

    char tooltip_sell[96];
    (void)snprintf(tooltip_sell, sizeof(tooltip_sell), "Sell %s ×%d.", resource_name(res), (int)qty);
    if (draw_button(renderer, font, "Sell", btn_sell_x, row_y + 5, btn_trade_w, btn_trade_h, can_sell_q, PAL_BTN_NORMAL, tooltip_sell)) {
      (void)sell_resource(state, res, qty);
    }

    char tooltip_buy[96];
    (void)snprintf(tooltip_buy, sizeof(tooltip_buy), "Buy %s ×%d.", resource_name(res), (int)qty);
    if (draw_button(renderer, font, "Buy", btn_buy_x, row_y + 5, btn_trade_w, btn_trade_h, can_buy_q, PAL_BTN_NORMAL, tooltip_buy)) {
      (void)buy_resource(state, res, qty);
    }
  }

  draw_label(renderer, font, "SHOP PERKS", perks_x, perks_y - 18, PAL_TEXT, ALIGN_LEFT);

  int perk_row_h = 54;
  int shown = 0;
  for (int id = 0; id < SHOP_COUNT; ++id) {
    ShopItemId item_id = (ShopItemId)id;

    // We don't expose defs publicly; mirror Lisp display rules via unlock/owned.
    // buy_shop_item() itself enforces availability and max-level.
    bool owned = shop_item_owned(state, item_id);

    // Use buy_shop_item with disabled button unless it would be visible in Lisp.
    // Visibility: owned or unlocked.
    // We approximate unlock by trying to buy? No. Instead, use save data: unlock is based on ever-produced.
    // We'll just show everything once unlocked via those thresholds.
    bool unlocked = false;
    switch (item_id) {
      case SHOP_HAGGLING:
        unlocked = resource_ever(state, RES_COINS) >= 120.0;
        break;
      case SHOP_SALESMANSHIP:
        unlocked = resource_ever(state, RES_COINS) >= 150.0;
        break;
      case SHOP_WAREHOUSE:
        unlocked = resource_ever(state, RES_STONE) >= 200.0;
        break;
      case SHOP_SMELTERS_COUPON:
        unlocked = resource_ever(state, RES_IRON_BAR) >= 5.0;
        break;
      case SHOP_FOREMAN_CONTRACT:
        unlocked = resource_ever(state, RES_BRONZE) >= 5.0;
        break;
      default:
        unlocked = false;
        break;
    }

    if (!owned && !unlocked) {
      continue;
    }

    int row_y = perks_y + shown * (perk_row_h + 10);
    if (row_y + perk_row_h >= y + h - 8) {
      break;
    }
    shown += 1;

    int lvl = shop_item_level(state, item_id);
    int max = 0;
    const char *name = "";
    const char *desc = "";
    double base_cost = 0.0;
    double scale = 1.0;
    switch (item_id) {
      case SHOP_HAGGLING:
        name = "Haggling Lessons";
        desc = "Shop prices -5% (stacks).";
        base_cost = 120.0;
        scale = 1.8;
        max = 10;
        break;
      case SHOP_SALESMANSHIP:
        name = "Salesmanship";
        desc = "Sell values +10% (stacks).";
        base_cost = 150.0;
        scale = 1.75;
        max = 12;
        break;
      case SHOP_WAREHOUSE:
        name = "Warehouse Lease";
        desc = "All storage caps +15% (stacks).";
        base_cost = 220.0;
        scale = 1.9;
        max = 8;
        break;
      case SHOP_SMELTERS_COUPON:
        name = "Smelter's Coupon";
        desc = "Smelting +10% speed (stacks).";
        base_cost = 400.0;
        scale = 1.85;
        max = 10;
        break;
      case SHOP_FOREMAN_CONTRACT:
        name = "Foreman Contract";
        desc = "All building production +7% (stacks).";
        base_cost = 650.0;
        scale = 2.0;
        max = 10;
        break;
      default:
        break;
    }

    bool maxed = max > 0 && lvl >= max;
    double cost = 0.0;
    if (!maxed) {
      cost = ceil(base_cost * pow(scale, (double)lvl) * state->shop_discount);
    }
    bool can_buy = !maxed && resource_has(state, RES_COINS, cost);

    int btn_w2 = 78;
    int btn_h2 = perk_row_h - 20;
    int btn_x2 = perks_x + perks_w - btn_w2 - 8;
    int btn_y2 = row_y + 10;
    int text_x = perks_x + 10;
    int text_right = btn_x2 - 10;

    char title[128];
    if (max > 1) {
      (void)snprintf(title, sizeof(title), "%s  (Lv %d/%d)", name, lvl, max);
    } else if (max == 1) {
      (void)snprintf(title, sizeof(title), "%s  (%s)", name, lvl > 0 ? "Owned" : "Available");
    } else {
      (void)snprintf(title, sizeof(title), "%s  (Lv %d)", name, lvl);
    }

    draw_rect_palette(renderer, perks_x, row_y, perks_w, perk_row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);
    draw_label_fit(renderer, font, title, text_x, row_y + 6, PAL_TEXT, text_right - text_x, ALIGN_LEFT);
    draw_label_fit(renderer, font, desc, text_x, row_y + 28, PAL_TEXT_DIM, text_right - text_x, ALIGN_LEFT);

    if (!maxed) {
      char cost_buf[64];
      fmt_number(cost_buf, sizeof(cost_buf), cost);
      char cost_line[96];
      (void)snprintf(cost_line, sizeof(cost_line), "Cost: %s c", cost_buf);
      draw_label(renderer, font, cost_line, text_right, row_y + 6, PAL_TEXT_DIM, ALIGN_RIGHT);
    }

    if (draw_button(renderer, font, maxed ? "Max" : "Buy", btn_x2, btn_y2, btn_w2, btn_h2, can_buy, PAL_BTN_NORMAL, maxed ? "Fully upgraded." : name)) {
      (void)buy_shop_item(state, item_id);
    }
  }
}

static void render_panel_prestige(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  (void)font_large;
  draw_panel_background(renderer, BG_PRESTIGE, x, y, w, h, (ColorRGBA){0, 0, 0, 160});
  draw_label(renderer, font_large, "PRESTIGE", x + 20, y + 10, PAL_TEXT, ALIGN_LEFT);

  char line[128];
  (void)snprintf(line, sizeof(line), "Ascensions: %d", state->ascensions);
  draw_label(renderer, font, line, x + 24, y + 64, PAL_TEXT, ALIGN_LEFT);

  char essence_buf[64];
  fmt_number(essence_buf, sizeof(essence_buf), state->essence);
  (void)snprintf(line, sizeof(line), "Ember Essence: %s", essence_buf);
  draw_label(renderer, font, line, x + 24, y + 88, PAL_TEXT, ALIGN_LEFT);

  char ember_buf[64];
  fmt_number(ember_buf, sizeof(ember_buf), resource_ever(state, RES_EMBER));
  (void)snprintf(line, sizeof(line), "Ember produced: %s", ember_buf);
  draw_label(renderer, font, line, x + 24, y + 112, PAL_TEXT_DIM, ALIGN_LEFT);

  bool can = prestige_available(state);
  if (can) {
    (void)snprintf(line,
                   sizeof(line),
                   "Gain %d Essence and reset (formula: floor(sqrt(ember)) + asc*2).",
                   essence_gain(state));
  } else {
    (void)snprintf(line, sizeof(line), "Prestige unlocks after producing 1 Ember Crystal.");
  }
  draw_label(renderer, font, line, x + 24, y + 140, PAL_TEXT_DIM, ALIGN_LEFT);

  char ascend_label[64];
  if (can) {
    (void)snprintf(ascend_label, sizeof(ascend_label), "ASCEND (+%d)", essence_gain(state));
  } else {
    (void)snprintf(ascend_label, sizeof(ascend_label), "ASCEND");
  }
  if (draw_button(renderer, font, ascend_label, x + 24, y + 190, 220, 44, can, PAL_BTN_NORMAL, NULL)) {
    (void)ascend(state);
  }

  int got = 0;
  for (int i = 0; i < ACH_COUNT; ++i) {
    if (state->achievements[i]) {
      got += 1;
    }
  }
  (void)snprintf(line, sizeof(line), "Achievements: %d/%d", got, ACH_COUNT);
  int ax = x + 24;
  int ay = y + 260;
  draw_label(renderer, font, line, ax, ay, PAL_TEXT, ALIGN_LEFT);

  int shown = 0;
  int yy = ay + 22;
  for (int i = 0; i < ACH_COUNT && shown < 8; ++i) {
    if (!state->achievements[i]) {
      continue;
    }
    const char *key = achievement_key((AchievementId)i);
    char a_line[128];
    (void)snprintf(a_line, sizeof(a_line), "✓ %s", key);
    draw_label(renderer, font, a_line, ax + 16, yy + (shown * 18), PAL_TEXT_DIM, ALIGN_LEFT);
    shown += 1;
  }

  int u_x = x + 420;
  int u_y = y + 64;
  int u_w = w - 444;
  int row_h = 52;
  int irow = 0;
  draw_label(renderer, font, "ETERNAL UPGRADES", u_x, u_y - 18, PAL_TEXT, ALIGN_LEFT);

  for (int id = 0; id < ETERNAL_COUNT; ++id) {
    EternalUpgradeId up_id = (EternalUpgradeId)id;
    bool owned = eternal_upgrade_owned(state, up_id);
    const char *name = "";
    const char *desc = "";
    double cost = 0.0;
    switch (up_id) {
      case ETERNAL_MINER:
        name = "Eternal Miner";
        desc = "Start each run with 1 free Pickaxe.";
        cost = 5.0;
        break;
      case ETERNAL_EMBER_MEMORY:
        name = "Ember Memory";
        desc = "Keep 10% of coins on Ascend.";
        cost = 10.0;
        break;
      case ETERNAL_FORGE_MASTERY:
        name = "Forge Mastery";
        desc = "All smelting is 25% faster permanently.";
        cost = 25.0;
        break;
      case ETERNAL_ANCIENT_VEINS:
        name = "Ancient Veins";
        desc = "+50% all ore production permanently.";
        cost = 50.0;
        break;
      case ETERNAL_TWIN_FURNACE:
        name = "Twin Furnaces";
        desc = "Start with Auto-Furnace already built.";
        cost = 100.0;
        break;
      default:
        break;
    }

    int row_y = u_y + irow * (row_h + 8);
    irow += 1;
    if (row_y + row_h >= y + h - 8) {
      break;
    }

    bool can_buy = !owned && state->essence >= cost;
    int btn_x = u_x + u_w - 92;
    int btn_y = row_y + 10;
    int btn_w = 84;
    int btn_h = row_h - 20;
    int cost_right = btn_x - 12;

    draw_rect_palette(renderer, u_x, row_y, u_w, row_h, PAL_BG_SIDEBAR, true, PAL_BTN_HOVER);

    char title[128];
    if (owned) {
      (void)snprintf(title, sizeof(title), "✓ %s", name);
    } else {
      (void)snprintf(title, sizeof(title), "%s", name);
    }
    draw_label(renderer, font, title, u_x + 10, row_y + 6, owned ? PAL_TEXT_DIM : PAL_TEXT, ALIGN_LEFT);
    draw_label(renderer, font, desc, u_x + 10, row_y + 26, PAL_TEXT_DIM, ALIGN_LEFT);

    char cost_buf[64];
    fmt_number(cost_buf, sizeof(cost_buf), cost);
    char cost_line[96];
    (void)snprintf(cost_line, sizeof(cost_line), "Cost: %s", cost_buf);
    draw_label(renderer, font, cost_line, cost_right, row_y + 6, PAL_TEXT_DIM, ALIGN_RIGHT);

    if (draw_button(renderer, font, owned ? "Owned" : "Buy", btn_x, btn_y, btn_w, btn_h, can_buy, PAL_BTN_NORMAL, NULL)) {
      (void)buy_eternal_upgrade(state, up_id);
    }
  }
}

static void render_main_panel(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large, int x, int y, int w, int h) {
  switch (state->active_panel) {
    case PANEL_FORGE:
      render_panel_forge(renderer, state, font, font_large, x, y, w, h);
      break;
    case PANEL_MINE:
      render_panel_mine(renderer, state, font, font_large, x, y, w, h);
      break;
    case PANEL_UPGRADES:
      render_panel_upgrades(renderer, state, font, font_large, x, y, w, h);
      break;
    case PANEL_RECIPES:
      render_panel_recipes(renderer, state, font, font_large, x, y, w, h);
      break;
    case PANEL_SHOP:
      render_panel_shop(renderer, state, font, font_large, x, y, w, h);
      break;
    case PANEL_PRESTIGE:
      render_panel_prestige(renderer, state, font, font_large, x, y, w, h);
      break;
    default:
      break;
  }
}

static void render_event_overlay(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large) {
  if (!state || state->active_event == EVENT_NONE) {
    return;
  }

  const EventDef *ev = event_def(state->active_event);
  if (!ev) {
    return;
  }

  draw_rect(renderer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (ColorRGBA){0, 0, 0, 180}, false, (ColorRGBA){0, 0, 0, 0});

  int pw = 760;
  int ph = 420;
  int px = (WINDOW_WIDTH - pw) / 2;
  int py = (WINDOW_HEIGHT - ph) / 2;

  draw_rect_palette(renderer, px, py, pw, ph, PAL_BG_PANEL, true, PAL_BTN_ACTIVE);
  draw_label(renderer, font_large, ev->name, px + 24, py + 18, PAL_TEXT, ALIGN_LEFT);
  draw_multiline_label(renderer, font, ev->text, px + 24, py + 64, PAL_TEXT_DIM, 18, ALIGN_LEFT);

  int btn_x = px + 24;
  int btn_w = pw - 48;
  int btn_h = 34;
  int btn_y = py + 200;

  for (size_t i = 0; i < ev->choice_count; ++i) {
    int yy = btn_y + (int)i * (btn_h + 10);
    const EventChoice *choice = &ev->choices[i];
    bool enabled = choice->can ? choice->can(state) : true;
    if (draw_button(renderer, font, choice->label, btn_x, yy, btn_w, btn_h, enabled, PAL_BTN_NORMAL, enabled ? NULL : "Insufficient resources.")) {
      (void)choose_active_event(state, i);
    }
  }
}

static void render_main_menu(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large) {
  draw_panel_background(renderer, BG_MENU, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (ColorRGBA){0, 0, 0, 190});

  int cx = WINDOW_WIDTH / 2;
  int title_y = 120;
  draw_label(renderer, font_large, "EMBER FORGE", cx, title_y, PAL_TEXT, ALIGN_CENTER);
  draw_label(renderer, font, "mine → smelt → build → ascend", cx, title_y + 30, PAL_TEXT_DIM, ALIGN_CENTER);

  /* ── Slot cards ─────────────────────────────────────────────────────────── */
  /* Cache timestamps; refresh at most once per second to avoid per-frame I/O */
  static uint64_t s_slot_ts[3]          = {0, 0, 0};
  static uint32_t s_slot_ts_refresh_ms  = 0;
  uint32_t now_ms = SDL_GetTicks();
  if (now_ms - s_slot_ts_refresh_ms > 1000 || s_slot_ts_refresh_ms == 0) {
    for (int i = 0; i < 3; ++i)
      s_slot_ts[i] = save_slot_saved_at(i + 1);
    s_slot_ts_refresh_ms = now_ms;
  }

  int box_w    = 460;
  int card_h   = 78;
  int card_gap = 10;
  int card_w   = (box_w - card_gap * 2) / 3;   /* 146 px */
  int cards_x0 = (WINDOW_WIDTH - box_w) / 2;
  int cards_y  = title_y + 68;

  for (int s = 1; s <= 3; ++s) {
    int cx2 = cards_x0 + (s - 1) * (card_w + card_gap);
    int cy  = cards_y;
    bool is_active = (state->save_slot == s);
    PaletteColor border = is_active ? PAL_BTN_ACTIVE : PAL_BTN_HOVER;

    draw_rect_palette(renderer, cx2, cy, card_w, card_h, PAL_BG_PANEL, true, border);

    /* "SLOT N" header */
    char hdr[12];
    (void)snprintf(hdr, sizeof(hdr), "SLOT %d", s);
    draw_label(renderer, font, hdr, cx2 + 8, cy + 6, PAL_TEXT_DIM, ALIGN_LEFT);

    /* Player-chosen name */
    const char *sname = state->slot_names[s - 1];
    if (sname[0]) {
      draw_label_fit(renderer, font, sname, cx2 + 8, cy + 26, PAL_TEXT, card_w - 16, ALIGN_LEFT);
    } else {
      draw_label(renderer, font, "Untitled", cx2 + 8, cy + 26, PAL_TEXT_DIM, ALIGN_LEFT);
    }

    /* Timestamp or "Empty" */
    char ts[64];
    format_timestamp(ts, sizeof(ts), s_slot_ts[s - 1]);
    draw_label_fit(renderer, font, ts, cx2 + 8, cy + 50, PAL_TEXT_DIM, card_w - 16, ALIGN_LEFT);

    /* Click → select slot */
    if (!ui.input_blocked && rect_hovered(cx2, cy, card_w, card_h)) {
      const char *tip = s_slot_ts[s - 1] ? "Select this save slot." : "Select this save slot (empty).";
      ui_set_tooltip(tip, ui.mouse_x, ui.mouse_y, TOOLTIP_MOUSE);
      if (ui_take_click() && state->save_slot != s) {
        ui_cancel_text_input();
        state->save_slot = s;
        (void)save_options(state);
      }
    }
  }

  /* ── Action buttons ─────────────────────────────────────────────────────── */
  int box_h = 28 + (4 * 40) + (3 * 12) + 28;   /* 4-button default height  */
  int btn_h = 40;
  int btn_gap = 12;
  int num_buttons = state->game_started ? 5 : 4;
  (void)num_buttons;
  if (state->game_started) box_h = 28 + (5 * btn_h) + (4 * btn_gap) + 28;
  int box_x = (WINDOW_WIDTH - box_w) / 2;
  int box_y = cards_y + card_h + 10;

  int btn_w = box_w - 48;
  int btn_x = box_x + 24;
  int btn_y = box_y + 28;
  int row_step = btn_h + btn_gap;

  draw_rect_palette(renderer, box_x, box_y, box_w, box_h, PAL_BG_PANEL, true, PAL_BTN_ACTIVE);

  if (state->game_started) {
    if (draw_button(renderer, font, "Resume", btn_x, btn_y, btn_w, btn_h, true, PAL_BTN_NORMAL, "Return to the forge.")) {
      state->screen = SCREEN_GAME;
    }
    if (draw_button(renderer, font, "Save", btn_x, btn_y + row_step, btn_w, btn_h, true, PAL_BTN_NORMAL, "Write the save now.")) {
      if (save_game(state)) {
        state->last_save_universal = universal_time_now();
        notifs_add(state, "Saved.", 3.0f, PAL_GREEN_OK);
        s_slot_ts_refresh_ms = 0; /* force timestamp refresh */
      }
    }
    if (draw_button(renderer, font, "Options", btn_x, btn_y + (2 * row_step), btn_w, btn_h, true, PAL_BTN_NORMAL, "Saves, autosave, and other settings.")) {
      state->screen = SCREEN_OPTIONS;
    }
    if (draw_button(renderer, font, "Return to Title", btn_x, btn_y + (3 * row_step), btn_w, btn_h, true, PAL_BTN_NORMAL, "Go back to the title screen.")) {
      state->game_started = false;
    }
    if (draw_button(renderer, font, "Quit", btn_x, btn_y + (4 * row_step), btn_w, btn_h, true, PAL_BTN_NORMAL, NULL)) {
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = SDL_QUIT;
      SDL_PushEvent(&ev);
    }
  } else {
    bool has_save = save_exists(state, state->save_slot);
    if (draw_button(renderer, font, "Continue", btn_x, btn_y, btn_w, btn_h, has_save, PAL_BTN_NORMAL, has_save ? "Load your save." : "No save found.")) {
      (void)start_continue_game(state);
    }
    if (draw_button(renderer, font, "New Game", btn_x, btn_y + row_step, btn_w, btn_h, true, PAL_BTN_NORMAL, "Start fresh (does not delete files until you save).")) {
      (void)start_new_game(state);
    }
    if (draw_button(renderer, font, "Options", btn_x, btn_y + (2 * row_step), btn_w, btn_h, true, PAL_BTN_NORMAL, "Saves, autosave, and other settings.")) {
      state->screen = SCREEN_OPTIONS;
    }
    if (draw_button(renderer, font, "Quit", btn_x, btn_y + (3 * row_step), btn_w, btn_h, true, PAL_BTN_NORMAL, NULL)) {
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = SDL_QUIT;
      SDL_PushEvent(&ev);
    }
  }
}

static void render_options_menu(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large) {
  draw_panel_background(renderer, BG_MENU, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (ColorRGBA){0, 0, 0, 190});

  int cx = WINDOW_WIDTH / 2;
  int title_y = 86;
  draw_label(renderer, font_large, "OPTIONS", cx, title_y, PAL_TEXT, ALIGN_CENTER);
  draw_label(renderer, font, "Saves, autosave, and gameplay settings.", cx, title_y + 30, PAL_TEXT_DIM, ALIGN_CENTER);

  int box_w = 920;
  int box_h = 560;
  int box_x = (WINDOW_WIDTH - box_w) / 2;
  int box_y = title_y + 70;
  draw_rect_palette(renderer, box_x, box_y, box_w, box_h, PAL_BG_PANEL, true, PAL_BTN_ACTIVE);

  int x0 = box_x + 24;
  int yy = box_y + 24;
  int row_h = 38;
  int label_w = 140;
  int btn_h = 30;
  int btn_gap = 10;

  draw_label(renderer, font, "SAVE", x0, yy, PAL_TEXT, ALIGN_LEFT);
  yy += 26;

  draw_label(renderer, font, "Slot:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  int slot_x = x0 + label_w;
  for (int s = 1; s <= 3; ++s) {
    int xx = slot_x + (s - 1) * 86;
    bool exists = save_exists(state, s);
    char label[8];
    (void)snprintf(label, sizeof(label), exists ? "%d*" : "%d", s);
    if (draw_button(renderer, font, label, xx, yy, 76, btn_h, true, (state->save_slot == s) ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL,
                    exists ? "Save found in this slot." : "Empty slot.")) {
      if (state->save_slot != s) {
        ui_cancel_text_input(); /* cancel rename when switching slots */
        state->save_slot = s;
        (void)save_options(state);
      }
    }
  }
  yy += row_h;

  /* ── Slot name text field ──────────────────────────────────────────────── */
  draw_label(renderer, font, "Name:", x0, yy + 5, PAL_TEXT_DIM, ALIGN_LEFT);
  {
    int field_x = x0 + label_w;
    int field_w = 300;
    int field_h = btn_h;
    bool field_hov = rect_hovered(field_x, yy, field_w, field_h);
    bool field_act = ui.text_input_active && (ui.text_input_slot == state->save_slot);
    PaletteColor bdr = field_act ? PAL_BTN_ACTIVE : (field_hov ? PAL_BTN_HOVER : PAL_TEXT_DIM);
    draw_rect_palette(renderer, field_x, yy, field_w, field_h, PAL_BG_PANEL, true, bdr);

    /* Choose what text to render inside the field */
    char cursor_buf[36];
    const char *display_text;
    if (field_act) {
      bool show_cur = (SDL_GetTicks() / 530) % 2 == 0;
      (void)snprintf(cursor_buf, sizeof(cursor_buf), "%s%s",
                     ui.text_input_buf, show_cur ? "|" : "");
      display_text = cursor_buf;
    } else {
      display_text = state->slot_names[state->save_slot - 1];
    }

    if (display_text && display_text[0]) {
      draw_label_fit(renderer, font, display_text, field_x + 7, yy + 6, PAL_TEXT, field_w - 14, ALIGN_LEFT);
    } else if (!field_act) {
      draw_label(renderer, font, "click to name this slot", field_x + 7, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
    }

    /* Click → activate text input */
    if (!ui.input_blocked && field_hov) {
      ui_set_tooltip("Type a name for this slot.  Enter to confirm, Esc to cancel.",
                     field_x, yy, TOOLTIP_ABOVE);
      if (ui_take_click() && !field_act) {
        ui.text_input_active = true;
        ui.text_input_slot   = state->save_slot;
        strncpy(ui.text_input_buf, state->slot_names[state->save_slot - 1], 31);
        ui.text_input_buf[31] = '\0';
        SDL_StartTextInput();
      }
    }
  }
  yy += row_h;

  char save_path[256];
  (void)snprintf(save_path, sizeof(save_path), "saves/slot%d/save.sav", state->save_slot);
  draw_label(renderer, font, "Save path:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  draw_label_fit(renderer, font, save_path, x0 + label_w, yy + 6, PAL_TEXT, box_w - label_w - 56, ALIGN_LEFT);
  yy += 22;

  draw_label(renderer, font, "Options:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  draw_label_fit(renderer, font, "saves/options.sav", x0 + label_w, yy + 6, PAL_TEXT, box_w - label_w - 56, ALIGN_LEFT);
  yy += row_h + 6;

  int btn_w = 170;
  bool has_save = save_exists(state, state->save_slot);
  bool can_load = (!state->game_started) && has_save;
  bool can_save = state->game_started;
  bool can_delete = (!state->game_started) && has_save;

  if (draw_button(renderer, font, "Save Now", x0, yy, btn_w, btn_h, can_save, PAL_BTN_NORMAL, can_save ? "Write save immediately." : "Start a game to save.")) {
    if (save_game(state)) {
      state->last_save_universal = universal_time_now();
      notifs_add(state, "Saved.", 3.0f, PAL_GREEN_OK);
    }
  }

  if (draw_button(renderer,
                  font,
                  "Load Slot",
                  x0 + (btn_w + btn_gap),
                  yy,
                  btn_w,
                  btn_h,
                  can_load,
                  PAL_BTN_NORMAL,
                  state->game_started ? "Return to title to load." : (has_save ? "Load this slot and start playing." : "No save in this slot."))) {
    (void)start_continue_game(state);
  }

  if (draw_button(renderer,
                  font,
                  "Delete Slot",
                  x0 + 2 * (btn_w + btn_gap),
                  yy,
                  btn_w,
                  btn_h,
                  can_delete,
                  PAL_BTN_NORMAL,
                  state->game_started ? "Return to title to delete saves." : (has_save ? "Delete this slot's save." : "No save in this slot."))) {
    if (delete_save(state, state->save_slot)) {
      notifs_add(state, "Deleted save.", 3.0f, PAL_TEXT_DIM);
    }
  }

  yy += row_h + 30;

  draw_label(renderer, font, "GAMEPLAY", x0, yy, PAL_TEXT, ALIGN_LEFT);
  yy += 26;

  draw_label(renderer, font, "Autosave:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  if (draw_button(renderer, font, state->autosave_enabled ? "On" : "Off", x0 + label_w, yy, 80, btn_h, true,
                  state->autosave_enabled ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL, "Autosave while playing.")) {
    state->autosave_enabled = !state->autosave_enabled;
    (void)save_options(state);
  }
  yy += row_h;

  draw_label(renderer, font, "Interval:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  int choices[] = {30, 60, 120};
  for (int i = 0; i < (int)EF_ARRAY_COUNT(choices); ++i) {
    int xx = x0 + label_w + i * 86;
    char label[16];
    (void)snprintf(label, sizeof(label), "%ds", choices[i]);
    bool active = state->autosave_interval_seconds == choices[i];
    if (draw_button(renderer, font, label, xx, yy, 76, btn_h, state->autosave_enabled, active ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL,
                    state->autosave_enabled ? "Autosave interval." : "Enable autosave first.")) {
      state->autosave_interval_seconds = choices[i];
      (void)save_options(state);
    }
  }
  yy += row_h;

  draw_label(renderer, font, "Offline:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  if (draw_button(renderer, font, state->offline_progress_enabled ? "On" : "Off", x0 + label_w, yy, 80, btn_h, true,
                  state->offline_progress_enabled ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL, "Apply offline production when loading a save.")) {
    state->offline_progress_enabled = !state->offline_progress_enabled;
    (void)save_options(state);
  }
  yy += row_h;

  draw_label(renderer, font, "Fullscreen:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  if (draw_button(renderer, font, state->fullscreen_enabled ? "On" : "Off", x0 + label_w, yy, 80, btn_h, true,
                  state->fullscreen_enabled ? PAL_BTN_ACTIVE : PAL_BTN_NORMAL, "Toggle fullscreen (F11).")) {
    state->fullscreen_enabled = !state->fullscreen_enabled;
    (void)save_options(state);
  }
  yy += row_h;

  draw_label(renderer, font, "Font:", x0, yy + 6, PAL_TEXT_DIM, ALIGN_LEFT);
  size_t n_fonts = 0;
  const char *const *fonts = available_fonts(&n_fonts);
  size_t idx = 0;
  for (size_t i = 0; i < n_fonts; ++i) {
    if (strcmp(state->font_path, fonts[i]) == 0) {
      idx = i;
      break;
    }
  }
  const char *label = n_fonts > 0 ? font_display_name(fonts[idx]) : "No fonts found";

  int font_btn_w = 34;
  if (draw_button(renderer, font, "<", x0 + label_w, yy, font_btn_w, btn_h, n_fonts > 1, PAL_BTN_NORMAL, "Previous font.")) {
    size_t new_idx = (idx + n_fonts - 1) % n_fonts;
    strncpy(state->font_path, fonts[new_idx], sizeof(state->font_path) - 1);
    state->font_path[sizeof(state->font_path) - 1] = '\0';
    (void)save_options(state);
  }
  if (draw_button(renderer, font, ">", x0 + label_w + (font_btn_w + btn_gap), yy, font_btn_w, btn_h, n_fonts > 1, PAL_BTN_NORMAL, "Next font.")) {
    size_t new_idx = (idx + 1) % n_fonts;
    strncpy(state->font_path, fonts[new_idx], sizeof(state->font_path) - 1);
    state->font_path[sizeof(state->font_path) - 1] = '\0';
    (void)save_options(state);
  }

  char font_label[256];
  if (n_fonts > 0) {
    (void)snprintf(font_label, sizeof(font_label), "%s (%zu/%zu)", label, idx + 1, n_fonts);
  } else {
    (void)snprintf(font_label, sizeof(font_label), "%s", label);
  }
  int font_label_x = x0 + label_w + (2 * (font_btn_w + btn_gap)) + 6;
  draw_label_fit(renderer, font, font_label, font_label_x, yy + 6, PAL_TEXT, box_w - (font_label_x - box_x) - 64, ALIGN_LEFT);

  int back_w = 140;
  int back_x = box_x + box_w - back_w - 24;
  int back_y = box_y + box_h - 54;
  if (draw_button(renderer, font, "Back", back_x, back_y, back_w, 34, true, PAL_BTN_NORMAL, "Return to the main menu.")) {
    ui_commit_text_input(state); /* commit any in-progress rename */
    (void)save_options(state);
    state->screen = SCREEN_MENU;
  }
}

void render_frame(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large) {
  set_render_color_palette(renderer, PAL_BG);
  SDL_RenderClear(renderer);

  switch (state->screen) {
    case SCREEN_MENU:
      ui.input_blocked = false;
      render_main_menu(renderer, state, font, font_large);
      render_notifications(renderer, font, state, 0, WINDOW_HEIGHT - 28, WINDOW_WIDTH, 28);
      render_tooltip(renderer, font);
      break;
    case SCREEN_OPTIONS:
      ui.input_blocked = false;
      render_options_menu(renderer, state, font, font_large);
      render_notifications(renderer, font, state, 0, WINDOW_HEIGHT - 28, WINDOW_WIDTH, 28);
      render_tooltip(renderer, font);
      break;
    case SCREEN_GAME: {
      bool modal = state->active_event != EVENT_NONE;
      int main_x = 0;
      int main_y = 64;
      int sidebar_w = 280;
      int main_w = WINDOW_WIDTH - sidebar_w;
      int main_h = WINDOW_HEIGHT - 64 - 28;
      int sidebar_x = main_w;
      int sidebar_y = 64;

      render_top_bar(renderer, state, font);
      ui.input_blocked = modal;
      render_nav_tabs(renderer, state, font);
      render_main_panel(renderer, state, font, font_large, main_x, main_y, main_w, main_h);
      render_resource_sidebar(renderer, state, font, sidebar_x, sidebar_y, sidebar_w, main_h);
      render_notifications(renderer, font, state, 0, WINDOW_HEIGHT - 28, WINDOW_WIDTH, 28);

      if (modal) {
        ui.input_blocked = false;
        render_event_overlay(renderer, state, font, font_large);
      }

      render_tooltip(renderer, font);
      break;
    }
    default:
      break;
  }

  SDL_RenderPresent(renderer);
}
