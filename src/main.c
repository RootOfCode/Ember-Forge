#include "ef_fonts.h"
#include "ef_game.h"
#include "ef_save.h"
#include "ef_state.h"
#include "ef_time.h"
#include "ef_ui.h"
#include "timer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#endif

typedef struct FontContext {
  TTF_Font *font;
  TTF_Font *font_large;
  char loaded_choice[1024];
} FontContext;

static void apply_fullscreen(SDL_Window *window, SDL_Renderer *renderer, bool enabled) {
  if (!window || !renderer) {
    return;
  }
  SDL_SetWindowFullscreen(window, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
}

static void window_to_logical(SDL_Window *window,
                              SDL_Renderer *renderer,
                              int window_x,
                              int window_y,
                              int *logical_x_out,
                              int *logical_y_out) {
  if (!window || !renderer || !logical_x_out || !logical_y_out) {
    return;
  }

  double max_x = (double)(WINDOW_WIDTH - 1);
  double max_y = (double)(WINDOW_HEIGHT - 1);

#if SDL_VERSION_ATLEAST(2, 0, 18)
  float logical_x = 0.0f;
  float logical_y = 0.0f;
  SDL_RenderWindowToLogical(renderer, window_x, window_y, &logical_x, &logical_y);
  if (logical_x < 0.0f) {
    logical_x = 0.0f;
  }
  if (logical_y < 0.0f) {
    logical_y = 0.0f;
  }
  if ((double)logical_x > max_x) {
    logical_x = (float)max_x;
  }
  if ((double)logical_y > max_y) {
    logical_y = (float)max_y;
  }
  *logical_x_out = (int)logical_x;
  *logical_y_out = (int)logical_y;
#else
  int window_width = 1;
  int window_height = 1;
  SDL_GetWindowSize(window, &window_width, &window_height);

  int output_width = 1;
  int output_height = 1;
  SDL_GetRendererOutputSize(renderer, &output_width, &output_height);

  if (window_width < 1) {
    window_width = 1;
  }
  if (window_height < 1) {
    window_height = 1;
  }
  if (output_width < 1) {
    output_width = 1;
  }
  if (output_height < 1) {
    output_height = 1;
  }

  double output_x = (double)window_x * ((double)output_width / (double)window_width);
  double output_y = (double)window_y * ((double)output_height / (double)window_height);

  double logical_x_double = (output_x * (double)WINDOW_WIDTH) / (double)output_width;
  double logical_y_double = (output_y * (double)WINDOW_HEIGHT) / (double)output_height;

  if (logical_x_double < 0.0) {
    logical_x_double = 0.0;
  }
  if (logical_y_double < 0.0) {
    logical_y_double = 0.0;
  }
  if (logical_x_double > max_x) {
    logical_x_double = max_x;
  }
  if (logical_y_double > max_y) {
    logical_y_double = max_y;
  }
  *logical_x_out = (int)logical_x_double;
  *logical_y_out = (int)logical_y_double;
#endif
}

static void add_fullscreen_notification(GameState *state) {
  if (!state) {
    return;
  }
  notifs_add(state, state->fullscreen_enabled ? "Fullscreen: On" : "Fullscreen: Off", 3.0f, PAL_TEXT_DIM);
}

static bool open_font_choice(FontContext *fonts, GameState *state, const char *choice) {
  if (!fonts || !state || !choice || choice[0] == '\0') {
    return false;
  }

  const char *file_path = resolve_font_file(choice);
  if (!file_path) {
    return false;
  }

  TTF_Font *new_font = TTF_OpenFont(file_path, 14);
  TTF_Font *new_font_large = TTF_OpenFont(file_path, 20);

  if (new_font && new_font_large) {
    if (fonts->font) {
      TTF_CloseFont(fonts->font);
    }
    if (fonts->font_large) {
      TTF_CloseFont(fonts->font_large);
    }
    fonts->font = new_font;
    fonts->font_large = new_font_large;
    strncpy(fonts->loaded_choice, choice, sizeof(fonts->loaded_choice) - 1);
    fonts->loaded_choice[sizeof(fonts->loaded_choice) - 1] = '\0';

    char msg[256];
    (void)snprintf(msg, sizeof(msg), "Font: %s", font_display_name(choice));
    notifs_add(state, msg, 3.0f, PAL_TEXT_DIM);
    return true;
  }

  if (new_font) {
    TTF_CloseFont(new_font);
  }
  if (new_font_large) {
    TTF_CloseFont(new_font_large);
  }
  return false;
}

static void ensure_fonts_loaded(FontContext *fonts, GameState *state) {
  if (!fonts || !state) {
    return;
  }

  if (state->font_path[0] != '\0' && open_font_choice(fonts, state, state->font_path)) {
    return;
  }

  const char *fallback = default_font_choice();
  strncpy(state->font_path, fallback, sizeof(state->font_path) - 1);
  state->font_path[sizeof(state->font_path) - 1] = '\0';
  if (open_font_choice(fonts, state, state->font_path)) {
    return;
  }
}

static void maybe_reload_fonts(FontContext *fonts, GameState *state) {
  if (!fonts || !state) {
    return;
  }
  if (state->font_path[0] == '\0') {
    return;
  }
  if (strcmp(state->font_path, fonts->loaded_choice) == 0) {
    return;
  }

  if (!open_font_choice(fonts, state, state->font_path)) {
    strncpy(state->font_path, fonts->loaded_choice, sizeof(state->font_path) - 1);
    state->font_path[sizeof(state->font_path) - 1] = '\0';
    notifs_add(state, "Failed to load font.", 3.0f, PAL_RUST);
  }
}

static void maybe_apply_fullscreen(SDL_Window *window, SDL_Renderer *renderer, const GameState *state, bool *fullscreen_applied) {
  if (!window || !renderer || !state || !fullscreen_applied) {
    return;
  }
  bool desired = state->fullscreen_enabled;
  if (desired == *fullscreen_applied) {
    return;
  }
  apply_fullscreen(window, renderer, desired);
  *fullscreen_applied = desired;
}

static bool handle_keydown(GameState *state, SDL_Window *window, SDL_Renderer *renderer, SDL_Scancode scancode) {
  (void)window;
  (void)renderer;
  if (!state) {
    return false;
  }

  /* When a slot-rename field is active, only editing keys are honoured.
     SDL_TEXTINPUT events handle printable characters separately. */
  if (ui_text_input_active()) {
    if (scancode == SDL_SCANCODE_BACKSPACE) {
      ui_feed_backspace();
      return true;
    }
    if (scancode == SDL_SCANCODE_RETURN || scancode == SDL_SCANCODE_KP_ENTER) {
      ui_commit_text_input(state);
      return true;
    }
    if (scancode == SDL_SCANCODE_ESCAPE) {
      ui_cancel_text_input();
      return true;
    }
    return true; /* swallow all other keys while typing */
  }

  if (scancode == SDL_SCANCODE_F11) {
    state->fullscreen_enabled = !state->fullscreen_enabled;
    (void)save_options(state);
    add_fullscreen_notification(state);
    return true;
  }

  if (state->active_event != EVENT_NONE) {
    return false;
  }

  if (state->screen == SCREEN_GAME) {
    switch (scancode) {
      case SDL_SCANCODE_1:
        state->active_panel = PANEL_FORGE;
        break;
      case SDL_SCANCODE_2:
        state->active_panel = PANEL_MINE;
        break;
      case SDL_SCANCODE_3:
        state->active_panel = PANEL_UPGRADES;
        break;
      case SDL_SCANCODE_4:
        state->active_panel = PANEL_RECIPES;
        break;
      case SDL_SCANCODE_5:
        state->active_panel = PANEL_SHOP;
        break;
      case SDL_SCANCODE_6:
        state->active_panel = PANEL_PRESTIGE;
        break;
      case SDL_SCANCODE_ESCAPE:
        state->screen = SCREEN_MENU;
        break;
      default:
        break;
    }
    return true;
  }

  if (state->screen == SCREEN_OPTIONS) {
    if (scancode == SDL_SCANCODE_ESCAPE) {
      (void)save_options(state);
      state->screen = SCREEN_MENU;
      return true;
    }
    return false;
  }

  if (scancode == SDL_SCANCODE_ESCAPE) {
    if (state->game_started) {
      state->screen = SCREEN_GAME;
    } else {
      SDL_Event quit_event;
      memset(&quit_event, 0, sizeof(quit_event));
      quit_event.type = SDL_QUIT;
      SDL_PushEvent(&quit_event);
    }
    return true;
  }

  if (scancode == SDL_SCANCODE_RETURN) {
    if (state->game_started) {
      state->screen = SCREEN_GAME;
      return true;
    }
    if (save_exists(state, state->save_slot)) {
      (void)start_continue_game(state);
      return true;
    }
  }

  return false;
}

static void run_game(void) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return;
  }

  if (TTF_Init() != 0) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    SDL_Quit();
    return;
  }

  int img_flags = IMG_INIT_PNG;
  if ((IMG_Init(img_flags) & img_flags) != img_flags) {
    fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
    TTF_Quit();
    SDL_Quit();
    return;
  }

  SDL_Window *window = SDL_CreateWindow("Ember Forge", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return;
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  GameState *state = game_state_create();
  if (!state) {
    fprintf(stderr, "Out of memory creating GameState.\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return;
  }

  bool clicked = false;
  bool prev_left_down = false;
  FontContext fonts;
  memset(&fonts, 0, sizeof(fonts));
  bool fullscreen_applied = false;

  (void)load_options(state);
  fullscreen_applied = state->fullscreen_enabled;
  apply_fullscreen(window, renderer, fullscreen_applied);

  scan_fonts();
  ensure_fonts_loaded(&fonts, state);

  timer_reset();

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          (void)save_options(state);
          if (state->game_started) {
            (void)save_game(state);
            state->last_save_universal = universal_time_now();
          }
          quit = true;
          break;
        case SDL_KEYDOWN:
          (void)handle_keydown(state, window, renderer, event.key.keysym.scancode);
          break;
        case SDL_TEXTINPUT:
          ui_feed_text(event.text.text);
          break;
        case SDL_MOUSEBUTTONDOWN:
          if (event.button.button == SDL_BUTTON_LEFT) {
            clicked = true;
          }
          break;
        default:
          break;
      }
    }

    float dt_seconds = timer_compute_dt_seconds();
    state->dt_seconds = dt_seconds;

    if (state->screen == SCREEN_GAME) {
      tick_game(state, dt_seconds);
    } else {
      update_fps(state, dt_seconds);
    }

    maybe_reload_fonts(&fonts, state);
    maybe_apply_fullscreen(window, renderer, state, &fullscreen_applied);

    int mouse_window_x = 0;
    int mouse_window_y = 0;
    uint32_t button_mask = (uint32_t)SDL_GetMouseState(&mouse_window_x, &mouse_window_y);
    bool left_down = (button_mask & SDL_BUTTON_LMASK) != 0u;
    bool edge_click = left_down && !prev_left_down;
    bool frame_click = clicked || edge_click;
    prev_left_down = left_down;

    int mouse_logical_x = 0;
    int mouse_logical_y = 0;
    window_to_logical(window, renderer, mouse_window_x, mouse_window_y, &mouse_logical_x, &mouse_logical_y);

    ui_begin_frame(mouse_logical_x, mouse_logical_y, frame_click);
    render_frame(renderer, state, fonts.font, fonts.font_large);

    clicked = false;

    if (state->screen == SCREEN_GAME) {
      maybe_autosave(state);
    }
  }

  clear_panel_backgrounds();
  fonts_shutdown();

  if (fonts.font) {
    TTF_CloseFont(fonts.font);
  }
  if (fonts.font_large) {
    TTF_CloseFont(fonts.font_large);
  }

  game_state_destroy(state);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  IMG_Quit();
  TTF_Quit();
  SDL_Quit();
}

int main(void) {
  run_game();
  return 0;
}
