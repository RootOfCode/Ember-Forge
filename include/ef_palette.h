#pragma once

#include <stdint.h>

typedef struct ColorRGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} ColorRGBA;

typedef enum PaletteColor {
  PAL_BG = 0,
  PAL_BG_PANEL = 1,
  PAL_BG_SIDEBAR = 2,
  PAL_BG_TOP = 3,
  PAL_BTN_NORMAL = 4,
  PAL_BTN_HOVER = 5,
  PAL_BTN_ACTIVE = 6,
  PAL_TEXT = 7,
  PAL_TEXT_DIM = 8,
  PAL_GOLD = 9,
  PAL_RUST = 10,
  PAL_ORANGE = 11,
  PAL_BRONZE = 12,
  PAL_SILVER = 13,
  PAL_BLUE = 14,
  PAL_CYAN = 15,
  PAL_RED = 16,
  PAL_GRAY = 17,
  PAL_DARK = 18,
  PAL_BROWN = 19,
  PAL_GREEN_OK = 20,
  PAL_WARN = 21,
} PaletteColor;

ColorRGBA palette_rgba(PaletteColor color);

