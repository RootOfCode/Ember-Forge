#pragma once

#include "ef_state.h"

#include <stdbool.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

void ui_begin_frame(int mouse_x, int mouse_y, bool clicked);
void ui_set_input_blocked(bool blocked);

/* Text input — called from the main event loop */
void ui_feed_text(const char *text);
void ui_feed_backspace(void);
bool ui_text_input_active(void);
void ui_commit_text_input(GameState *state);
void ui_cancel_text_input(void);

void clear_panel_backgrounds(void);

void render_frame(SDL_Renderer *renderer, GameState *state, TTF_Font *font, TTF_Font *font_large);

