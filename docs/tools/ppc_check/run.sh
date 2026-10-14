#!/usr/bin/env bash
# Builds the helpers on the host (stub ESPHome headers from ../biquad_check) and compares every biquad of the
# TI PurePath Console reference cfg (50, 315, 800, 1250, 3150 and 8000 Hz) with the helpers.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../../../components/tas58xx"
OUT="$(mktemp -d)"
g++ -std=gnu++20 -O2 -I"$HERE/../biquad_check/stub" -I"$SRC" "$HERE/cfgtest.cpp" "$SRC/tas58xx_helpers.cpp" -o "$OUT/cfgtest"
python3 "$HERE/compare.py" "$OUT/cfgtest"
rm -rf "$OUT"
