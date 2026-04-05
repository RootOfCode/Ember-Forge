#pragma once

#include <stdint.h>
#include <time.h>

#define UNIVERSAL_TIME_UNIX_EPOCH_OFFSET ((uint64_t)2208988800u)

static inline uint64_t unix_to_universal(time_t unix_seconds) {
  return (uint64_t)unix_seconds + (uint64_t)UNIVERSAL_TIME_UNIX_EPOCH_OFFSET;
}

static inline time_t universal_to_unix(uint64_t universal_seconds) {
  if (universal_seconds < (uint64_t)UNIVERSAL_TIME_UNIX_EPOCH_OFFSET) {
    return (time_t)0;
  }
  return (time_t)(universal_seconds - (uint64_t)UNIVERSAL_TIME_UNIX_EPOCH_OFFSET);
}

uint64_t universal_time_now(void);

