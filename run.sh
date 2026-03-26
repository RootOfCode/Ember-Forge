#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

if [[ ! -f "$HOME/quicklisp/setup.lisp" ]]; then
  echo "Quicklisp not found at: $HOME/quicklisp/setup.lisp" >&2
  echo "Install Quicklisp, then re-run ./run.sh" >&2
  exit 1
fi

export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$ROOT_DIR/.cache}"
mkdir -p "$XDG_CACHE_HOME"

sbcl --noinform --non-interactive \
  --eval '(load (merge-pathnames "quicklisp/setup.lisp" (user-homedir-pathname)))' \
  --eval '(require :asdf)' \
  --eval '(asdf:load-asd (truename "ember-forge.asd"))' \
  --eval '(asdf:load-system :ember-forge)' \
  --eval '(ember-forge:main)' \
  --quit

