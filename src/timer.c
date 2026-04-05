#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

static Uint32 last_ticks = 0;

void timer_reset(void) {
  last_ticks = SDL_GetTicks();
}

float timer_compute_dt_seconds(void) {
  Uint32 now_ticks = SDL_GetTicks();
  float dt_seconds = (float)(now_ticks - last_ticks) / 1000.0f;
  last_ticks = now_ticks;
  if (dt_seconds > 0.5f) {
    dt_seconds = 0.5f;
  }
  return dt_seconds;
}

