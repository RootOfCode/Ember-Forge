#include "ef_time.h"

#include <time.h>

uint64_t universal_time_now(void) {
  time_t now_unix = time(NULL);
  if (now_unix < 0) {
    now_unix = 0;
  }
  return unix_to_universal(now_unix);
}

