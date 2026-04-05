#include "ef_palette.h"

ColorRGBA palette_rgba(PaletteColor color) {
  switch (color) {
    case PAL_BG:
      return (ColorRGBA){18, 16, 20, 255};
    case PAL_BG_PANEL:
      return (ColorRGBA){28, 25, 32, 255};
    case PAL_BG_SIDEBAR:
      return (ColorRGBA){22, 20, 26, 255};
    case PAL_BG_TOP:
      return (ColorRGBA){24, 22, 28, 255};
    case PAL_BTN_NORMAL:
      return (ColorRGBA){55, 50, 65, 255};
    case PAL_BTN_HOVER:
      return (ColorRGBA){80, 72, 95, 255};
    case PAL_BTN_ACTIVE:
      return (ColorRGBA){110, 95, 130, 255};
    case PAL_TEXT:
      return (ColorRGBA){220, 215, 230, 255};
    case PAL_TEXT_DIM:
      return (ColorRGBA){130, 120, 145, 255};
    case PAL_GOLD:
      return (ColorRGBA){220, 180, 50, 255};
    case PAL_RUST:
      return (ColorRGBA){180, 80, 40, 255};
    case PAL_ORANGE:
      return (ColorRGBA){200, 120, 30, 255};
    case PAL_BRONZE:
      return (ColorRGBA){160, 100, 40, 255};
    case PAL_SILVER:
      return (ColorRGBA){180, 185, 195, 255};
    case PAL_BLUE:
      return (ColorRGBA){80, 140, 200, 255};
    case PAL_CYAN:
      return (ColorRGBA){60, 200, 190, 255};
    case PAL_RED:
      return (ColorRGBA){210, 60, 60, 255};
    case PAL_GRAY:
      return (ColorRGBA){120, 120, 130, 255};
    case PAL_DARK:
      return (ColorRGBA){40, 38, 45, 255};
    case PAL_BROWN:
      return (ColorRGBA){110, 70, 40, 255};
    case PAL_GREEN_OK:
      return (ColorRGBA){60, 180, 80, 255};
    case PAL_WARN:
      return (ColorRGBA){200, 160, 40, 255};
    default:
      return (ColorRGBA){255, 0, 255, 255};
  }
}

