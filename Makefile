# =============================================================================
# Ember Forge — Cross-platform Makefile
# =============================================================================
#
# Targets
#   make              Build for the current host platform (auto-detected)
#   make linux        Build for Linux
#   make macos        Build for macOS (Homebrew or MacPorts SDL2)
#   make windows      Cross-compile for Windows with MinGW-w64
#   make clean        Remove obj/ and bin/
#   make dirs         Create obj/ and bin/ directories
#
# Variables (override on command line)
#   CC        Compiler           e.g.  make CC=clang
#   OPT       Optimisation flag  e.g.  make OPT=-O0
#   V         Verbosity          V=1 shows every command, V=0 (default) is terse
#   SDL2_MINGW_PREFIX  Path to SDL2 MinGW headers/libs (Windows cross-compile)
#
# Quick examples
#   make                                 # build for current OS
#   make windows                         # cross-compile .exe from Linux/macOS
#   make OPT=-O0 V=1                     # verbose debug build
#   make CC=clang linux                  # force clang on Linux
# =============================================================================

# ── Binary name ───────────────────────────────────────────────────────────
TARGET_BASE := ember_forge

# ── Directory layout ──────────────────────────────────────────────────────
SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj
BIN_DIR  := bin

# ── Sources and objects ───────────────────────────────────────────────────
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# ── Verbosity ─────────────────────────────────────────────────────────────
V ?= 0
ifeq ($(V),1)
    Q   :=
    SAY := @true
else
    Q   := @
    SAY := @echo
endif

# ── Common flags ──────────────────────────────────────────────────────────
OPT          ?= -O2
BASE_CFLAGS  := $(OPT) -std=c11 -Wall -Wextra -Wpedantic -I$(INC_DIR)

# =============================================================================
# Auto-detect host platform
# =============================================================================
ifeq ($(OS),Windows_NT)
    HOST_PLATFORM := windows
else
    _UNAME := $(shell uname -s 2>/dev/null)
    ifeq ($(_UNAME),Darwin)
        HOST_PLATFORM := macos
    else
        HOST_PLATFORM := linux
    endif
endif

.PHONY: all
all: $(HOST_PLATFORM)

# =============================================================================
# Linux
# =============================================================================
.PHONY: linux
linux: export CC      := $(or $(CC),gcc)
linux: export CFLAGS  := $(BASE_CFLAGS) $(shell sdl2-config --cflags)
linux: export LDFLAGS := $(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lm
linux: export EF_TARGET := $(BIN_DIR)/$(TARGET_BASE)
linux: dirs
	$(MAKE) _build EF_TARGET=$(BIN_DIR)/$(TARGET_BASE)
	$(SAY) "  [Linux] Build complete → $(BIN_DIR)/$(TARGET_BASE)"

# =============================================================================
# macOS
# =============================================================================
.PHONY: macos
macos: export CC     := $(or $(CC),$(shell command -v clang 2>/dev/null || echo gcc))
macos: _BREW         := $(shell brew --prefix 2>/dev/null || echo /usr/local)
macos: _SDL2CFG      := $(firstword $(wildcard $(_BREW)/bin/sdl2-config) $(shell command -v sdl2-config 2>/dev/null) sdl2-config)
macos: export CFLAGS := $(BASE_CFLAGS) $(shell $(_SDL2CFG) --cflags 2>/dev/null || echo "-I$(_BREW)/include/SDL2 -D_THREAD_SAFE")
macos: export LDFLAGS:= $(shell $(_SDL2CFG) --libs 2>/dev/null || echo "-L$(_BREW)/lib -lSDL2") -lSDL2_image -lSDL2_ttf -lm
macos: dirs
	$(MAKE) _build EF_TARGET=$(BIN_DIR)/$(TARGET_BASE)
	$(SAY) "  [macOS] Build complete → $(BIN_DIR)/$(TARGET_BASE)"

# =============================================================================
# Windows — cross-compile with MinGW-w64 from Linux or macOS
# =============================================================================
# Install toolchain (Debian/Ubuntu):
#   sudo apt install gcc-mingw-w64-x86-64
#   sudo apt install libsdl2-dev:mingw-w64 libsdl2-image-dev:mingw-w64 libsdl2-ttf-dev:mingw-w64
#
# Or point at a downloaded SDL2 MinGW bundle:
#   make windows SDL2_MINGW_PREFIX=/path/to/SDL2-2.x.x-win32-x86_64
SDL2_MINGW_PREFIX ?= $(firstword \
    $(wildcard /usr/x86_64-w64-mingw32) \
    $(wildcard /usr/local/x86_64-w64-mingw32) \
    /usr/x86_64-w64-mingw32)

.PHONY: windows
windows: export CC      := x86_64-w64-mingw32-gcc
windows: export CFLAGS  := $(BASE_CFLAGS) -I$(SDL2_MINGW_PREFIX)/include/SDL2 -D_WIN32
windows: export LDFLAGS := -L$(SDL2_MINGW_PREFIX)/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lm -mwindows
windows: dirs
	$(MAKE) _build EF_TARGET=$(BIN_DIR)/$(TARGET_BASE).exe
	$(SAY) "  [Windows] Build complete → $(BIN_DIR)/$(TARGET_BASE).exe"

# =============================================================================
# Internal build sub-target (called by platform targets)
# =============================================================================
.PHONY: _build
_build: $(EF_TARGET)

$(EF_TARGET): $(OBJS)
	$(SAY) "  LINK  $@"
	$(Q)$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(SAY) "  CC    $<"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# =============================================================================
# Utilities
# =============================================================================
.PHONY: dirs
dirs:
	$(Q)mkdir -p $(OBJ_DIR) $(BIN_DIR)

.PHONY: clean
clean:
	$(SAY) "  CLEAN"
	$(Q)rm -rf $(OBJ_DIR) $(BIN_DIR)
