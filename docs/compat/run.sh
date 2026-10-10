#!/usr/bin/env bash
# Validates every YAML in docs/compat/cases with `esphome config` against a tas58xx component.
# Usage: docs/compat/run.sh [DIR]
#   DIR = directory that contains the tas58xx component folder (default: <repo>/components)
#   Requires `esphome` on PATH (ESPHome 2026.10.0+ for dev_update_with_onaudio).
# Compare against main:
#   git worktree add /tmp/tas58xx-main main && docs/compat/run.sh /tmp/tas58xx-main/components
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
COMPDIR="$(cd "${1:-$HERE/../../components}" && pwd)"
WORK="$(mktemp -d)"
for f in "$HERE"/cases/*.yaml; do
  name="$(basename "$f" .yaml)"
  sed "s#COMPDIR#$COMPDIR#" "$f" > "$WORK/$name.yaml"
  out="$(esphome config "$WORK/$name.yaml" 2>&1)"
  if echo "$out" | grep -q 'Configuration is valid'; then
    warn="$(echo "$out" | grep -o 'WARNING .*' | grep -v 'esphome/components' | head -1)"
    echo "$name: VALID ${warn:+ ($warn)}"
  else
    echo "$name: FAIL :: $(echo "$out" | grep -E 'invalid|Invalid|required|not allowed|Cannot' | head -1 | tr -s ' ')"
  fi
done
rm -rf "$WORK"
