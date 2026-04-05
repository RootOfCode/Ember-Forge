#include "ef_fonts.h"
#include "ef_platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **fonts     = NULL;
static size_t fonts_len = 0;
static size_t fonts_cap = 0;
static bool   scanned   = false;

static bool str_has_font_ext(const char *name) {
    if (!name) return false;
    const char *dot = strrchr(name, '.');
    if (!dot || dot[1] == '\0') return false;
    const char *ext = dot + 1;
    return ef_strcasecmp(ext, "ttf") == 0 || ef_strcasecmp(ext, "otf") == 0;
}

static bool vec_push(char ***vec, size_t *len, size_t *cap, const char *value) {
    if (!vec || !len || !cap || !value) return false;
    if (*len + 1 > *cap) {
        size_t nc = *cap > 0 ? *cap * 2 : 64;
        void *nm = realloc(*vec, nc * sizeof((*vec)[0]));
        if (!nm) return false;
        *vec = (char **)nm;
        *cap = nc;
    }
    size_t vl = strlen(value);
    char *copy = (char *)malloc(vl + 1);
    if (!copy) return false;
    memcpy(copy, value, vl + 1);
    (*vec)[(*len)++] = copy;
    return true;
}

static void scan_dir_recursive(const char *root) {
    if (!root || !ef_path_is_dir(root)) return;

    DIR *dir = opendir(root);
    if (!dir) return;

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char path[2048];
        (void)snprintf(path, sizeof(path), "%s" EF_PATH_SEP "%s", root, ent->d_name);

        if (ef_path_is_dir(path)) {
            scan_dir_recursive(path);
            continue;
        }
        if (!ef_path_is_file(path)) continue;
        if (!str_has_font_ext(ent->d_name)) continue;
        (void)vec_push(&fonts, &fonts_len, &fonts_cap, path);
    }
    closedir(dir);
}

static int cmp_str_ptr(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

void scan_fonts(void) {
    if (scanned) return;
    scanned = true;
    scan_dir_recursive("assets" EF_PATH_SEP "fonts");
    scan_dir_recursive("fonts");
    if (fonts_len == 0) return;
    qsort(fonts, fonts_len, sizeof(fonts[0]), cmp_str_ptr);
    size_t out = 0;
    for (size_t i = 0; i < fonts_len; ++i) {
        if (out == 0 || strcmp(fonts[i], fonts[out - 1]) != 0) fonts[out++] = fonts[i];
        else free(fonts[i]);
    }
    fonts_len = out;
}

void fonts_shutdown(void) {
    for (size_t i = 0; i < fonts_len; ++i) free(fonts[i]);
    free(fonts);
    fonts = NULL; fonts_len = 0; fonts_cap = 0; scanned = false;
}

const char *const *available_fonts(size_t *out_count) {
    scan_fonts();
    if (out_count) *out_count = fonts_len;
    return (const char *const *)fonts;
}

const char *resolve_font_file(const char *choice) {
    if (!choice || choice[0] == '\0') return NULL;
    return ef_path_is_file(choice) ? choice : NULL;
}

const char *font_display_name(const char *choice) {
    if (!choice) return "";
    const char *slash = strrchr(choice, '/');
    if (!slash) slash = strrchr(choice, '\\');
    if (slash && slash[1] != '\0') return slash + 1;
    return choice;
}

const char *default_font_choice(void) {
    static const char *preferred = "assets" EF_PATH_SEP "fonts"
                                   EF_PATH_SEP "Roboto-Mono"
                                   EF_PATH_SEP "Roboto-Mono-regular.ttf";
    size_t count = 0;
    const char *const *list = available_fonts(&count);
    for (size_t i = 0; i < count; ++i)
        if (strcmp(list[i], preferred) == 0) return preferred;
    return count > 0 ? list[0] : preferred;
}
