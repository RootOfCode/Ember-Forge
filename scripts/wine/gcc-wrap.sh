#!/usr/bin/env bash
set -euo pipefail

winpath_to_unix() {
  local p="$1"

  # Only attempt to convert obvious Windows paths.
  if [[ "$p" =~ ^[A-Za-z]:[\\/].* ]]; then
    local winepath_cmd="${EMBER_FORGE_WINEPATH:-}"
    if [[ -z "$winepath_cmd" ]]; then
      if [[ -x "/usr/bin/winepath" ]]; then
        winepath_cmd="/usr/bin/winepath"
      elif command -v winepath >/dev/null 2>&1; then
        winepath_cmd="winepath"
      fi
    fi

    if [[ -n "$winepath_cmd" ]]; then
      # winepath understands both backslashes and forward slashes.
      local out
      if out="$("$winepath_cmd" -u "$p" 2>/dev/null)"; then
        if [[ -n "$out" ]]; then
          printf '%s' "$out"
          return 0
        fi
      fi
    fi
  fi

  printf '%s' "$p"
}

convert_arg() {
  local a="$1"

  case "$a" in
    -I*|-L*)
      local prefix="${a:0:2}"
      local path="${a:2}"
      if [[ -n "$path" ]]; then
        printf '%s%s' "$prefix" "$(winpath_to_unix "$path")"
      else
        printf '%s' "$a"
      fi
      ;;
    *)
      # Standalone file paths.
      if [[ "$a" =~ ^[A-Za-z]:[\\/].* ]]; then
        winpath_to_unix "$a"
      else
        printf '%s' "$a"
      fi
      ;;
  esac
}

compiler="${EMBER_FORGE_MINGW_CC:-}"
if [[ -z "$compiler" ]]; then
  if [[ -x "/usr/bin/x86_64-w64-mingw32-gcc" ]]; then
    compiler="/usr/bin/x86_64-w64-mingw32-gcc"
  else
    compiler="x86_64-w64-mingw32-gcc"
  fi
fi

out=()
expect_path_after=""
for arg in "$@"; do
  if [[ -n "$expect_path_after" ]]; then
    out+=("$(convert_arg "$arg")")
    expect_path_after=""
    continue
  fi

  case "$arg" in
    -I|-L)
      out+=("$arg")
      expect_path_after="$arg"
      ;;
    -o|-MF|-MT|-MQ)
      out+=("$arg")
      expect_path_after="$arg"
      ;;
    *)
      out+=("$(convert_arg "$arg")")
      ;;
  esac
done

exec "$compiler" "${out[@]}"
