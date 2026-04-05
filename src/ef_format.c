#include "ef_format.h"

#include <math.h>
#include <stdio.h>

static const char *const number_suffixes[] = {"K", "M", "B", "T", "Qa", "Qi", "Sx", "Sp", "Oc", "No", "Dc"};

void fmt_number(char *buf, size_t buf_len, double n) {
  if (!buf || buf_len == 0) {
    return;
  }

  double absn = fabs(n);
  if (absn < 1000.0) {
    (void)snprintf(buf, buf_len, "%.1f", n);
    return;
  }

  if (absn < 1.0e36) {
    double log10n = log10(absn);
    int exp3 = (int)floor(log10n / 3.0);
    int idx = exp3 < 1 ? 1 : exp3;
    int suffix_idx = idx - 1;
    int suffix_max = (int)(sizeof(number_suffixes) / sizeof(number_suffixes[0])) - 1;
    if (suffix_idx > suffix_max) {
      suffix_idx = suffix_max;
      idx = suffix_idx + 1;
    }
    double scaled = n / pow(1000.0, (double)idx);
    (void)snprintf(buf, buf_len, "%.2f%s", scaled, number_suffixes[suffix_idx]);
    return;
  }

  (void)snprintf(buf, buf_len, "%.3E", n);
}

void fmt_rate(char *buf, size_t buf_len, double n) {
  if (!buf || buf_len == 0) {
    return;
  }
  char tmp[64];
  fmt_number(tmp, sizeof(tmp), n);
  (void)snprintf(buf, buf_len, "%s/s", tmp);
}

