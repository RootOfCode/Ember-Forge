#pragma once

#include <stddef.h>

void scan_fonts(void);
void fonts_shutdown(void);

const char *const *available_fonts(size_t *out_count);
const char *resolve_font_file(const char *choice);
const char *font_display_name(const char *choice);
const char *default_font_choice(void);

