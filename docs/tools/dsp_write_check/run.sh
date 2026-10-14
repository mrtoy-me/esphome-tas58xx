#!/usr/bin/env bash
# Simulates the DSP memory (book, page, subaddress) and checks dsp_write_bytes_ in tas58xx.cpp against the
# per biquad writer it replaced (biquad_write_bytes_ at 2749ff8): every run of consecutive biquads on both
# channels, at the TAS5805M (page 0x24 subaddress 0x18) and TAS5825M (page 0x01 subaddress 0x30) start
# addresses, must leave the same memory, end on book 0 page 0, use I2C writes of at most 16 bytes that start
# on a coefficient boundary and never run past the end of a page.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$HERE/../../.."
OUT="$(mktemp -d)"
extract() {  # file content on stdin, function signature start
  python3 -c 'import sys; s=sys.stdin.read(); a=s.index(sys.argv[1]); print(s[a:s.index("\n}\n",a)+3])' "$1"
}
git -C "$REPO" show 2749ff8:components/tas58xx/tas58xx.cpp | extract 'bool Tas58xxComponent::biquad_write_bytes_(' > "$OUT/old_fn.inc"
extract 'bool Tas58xxComponent::dsp_write_bytes_(' < "$REPO/components/tas58xx/tas58xx.cpp" > "$OUT/new_fn.inc"
cp "$HERE/sim.cpp" "$OUT/"
g++ -std=gnu++20 -O1 -Wall -fsanitize=address,undefined "$OUT/sim.cpp" -o "$OUT/sim"
"$OUT/sim"
rm -rf "$OUT"
