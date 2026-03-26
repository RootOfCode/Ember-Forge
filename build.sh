#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

if [[ ! -f "$HOME/quicklisp/setup.lisp" ]]; then
  echo "Quicklisp not found at: $HOME/quicklisp/setup.lisp" >&2
  echo "Install Quicklisp, then re-run ./build.sh" >&2
  exit 1
fi

export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$ROOT_DIR/.cache}"
mkdir -p "$XDG_CACHE_HOME"

mkdir -p dist
rm -rf dist/assets
cp -R assets dist/assets

export EMBER_FORGE_OUT="dist/ember-forge"
export EMBER_FORGE_COMPRESSION="1"
sbcl --noinform --non-interactive --load scripts/build-release.lisp --quit

echo "Built: dist/ember-forge"
