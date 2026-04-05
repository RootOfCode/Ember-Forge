/*
 * ef_platform.h — Cross-platform shims
 *
 * Abstracts the subset of POSIX/Win32 that Ember Forge uses:
 *   - Directory creation / removal  (mkdir / rmdir)
 *   - File-existence check          (stat)
 *   - Case-insensitive string compare (strcasecmp)
 *   - Directory enumeration         (dirent / FindFirstFile)
 *   - Path separator constant       (EF_PATH_SEP)
 *
 * Every other source file should include this instead of pulling in
 * <unistd.h>, <sys/stat.h>, <strings.h>, or <dirent.h> directly.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Windows
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifdef _WIN32

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <direct.h>   /* _mkdir, _rmdir */
#  include <io.h>       /* _access */
#  include <sys/stat.h> /* _stat */

/* Directory create / remove */
static inline int ef_mkdir(const char *path) {
    return _mkdir(path);
}
static inline int ef_rmdir(const char *path) {
    return _rmdir(path);
}

/* File / path existence using Win32 stat */
static inline bool ef_path_exists(const char *path) {
    struct _stat st;
    return path && _stat(path, &st) == 0;
}
static inline bool ef_path_is_dir(const char *path) {
    struct _stat st;
    return path && _stat(path, &st) == 0 && (st.st_mode & _S_IFDIR);
}
static inline bool ef_path_is_file(const char *path) {
    struct _stat st;
    return path && _stat(path, &st) == 0 && (st.st_mode & _S_IFREG);
}

/* Case-insensitive compare */
#  define ef_strcasecmp  _stricmp
#  define ef_strncasecmp _strnicmp

/* Path separator */
#  define EF_PATH_SEP "\\"
#  define EF_PATH_SEP_CHAR '\\'

/* ── Directory enumeration for Windows ─────────────────────────────────── *
 * Provides a minimal dirent-compatible API backed by FindFirstFile so     *
 * ef_fonts.c can use the same code on all platforms.                      */

struct dirent {
    char d_name[MAX_PATH];
};

typedef struct {
    HANDLE          handle;
    WIN32_FIND_DATAA find_data;
    struct dirent   entry;
    bool            first;
} DIR;

static inline DIR *opendir(const char *path) {
    if (!path) return NULL;
    char pattern[MAX_PATH];
    (void)snprintf(pattern, sizeof(pattern), "%s\\*", path);
    DIR *d = (DIR *)calloc(1, sizeof(DIR));
    if (!d) return NULL;
    d->handle = FindFirstFileA(pattern, &d->find_data);
    if (d->handle == INVALID_HANDLE_VALUE) { free(d); return NULL; }
    d->first = true;
    return d;
}

static inline struct dirent *readdir(DIR *d) {
    if (!d) return NULL;
    if (d->first) {
        d->first = false;
    } else {
        if (!FindNextFileA(d->handle, &d->find_data)) return NULL;
    }
    strncpy(d->entry.d_name, d->find_data.cFileName, MAX_PATH - 1);
    d->entry.d_name[MAX_PATH - 1] = '\0';
    return &d->entry;
}

static inline int closedir(DIR *d) {
    if (!d) return -1;
    FindClose(d->handle);
    free(d);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * POSIX (Linux / macOS)
 * ═══════════════════════════════════════════════════════════════════════════ */
#else  /* !_WIN32 */

#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <strings.h>  /* strcasecmp */

static inline int ef_mkdir(const char *path) {
    return mkdir(path, 0755);
}
static inline int ef_rmdir(const char *path) {
    return rmdir(path);
}

static inline bool ef_path_exists(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0;
}
static inline bool ef_path_is_dir(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
static inline bool ef_path_is_file(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

#  define ef_strcasecmp  strcasecmp
#  define ef_strncasecmp strncasecmp

#  define EF_PATH_SEP "/"
#  define EF_PATH_SEP_CHAR '/'

#endif /* _WIN32 */

/* ═══════════════════════════════════════════════════════════════════════════
 * Shared helpers (work on both platforms once the above is resolved)
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Create a directory, succeed if it already exists. */
static inline bool ef_ensure_dir(const char *path) {
    if (!path || path[0] == '\0') return false;
    if (ef_mkdir(path) == 0) return true;
    return errno == EEXIST;
}
