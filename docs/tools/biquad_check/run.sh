#!/usr/bin/env bash
# Builds tas58xx_helpers.cpp on the host with stub ESPHome headers and dumps every biquad (all filter types,
# 48 frequencies, gains -24 to 24, 13 Q values, 3 sample rates) and the 9.23 gains for -48 to 48 dB.
# A change to the helpers that keeps the coefficients bit for bit gives the same md5 as the reference below.
# Reference: tas58xx_helpers.cpp at 8570b14 (before the helper cleanup), g++ -O0 and -O2 on x86-64.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../../../components/tas58xx"
OUT="$(mktemp -d)"
g++ -std=gnu++20 -O2 -I"$HERE/stub" -I"$SRC" "$HERE/dump.cpp" "$SRC/tas58xx_helpers.cpp" -o "$OUT/dump"
"$OUT/dump" > "$OUT/biquads.txt"
echo "lines: $(wc -l < "$OUT/biquads.txt")  (reference 642194)"
echo "md5:   $(md5sum "$OUT/biquads.txt" | cut -d' ' -f1)  (reference 8851b8199342fcbccfffbdac2e6b5c1b)"
echo "output kept in $OUT/biquads.txt"
