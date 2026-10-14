#!/usr/bin/env bash
# Builds the helpers on the host (stub ESPHome headers from ../biquad_check) and compares every biquad of the
# TI PurePath Console reference cfg (50, 315, 800, 1250, 3150 and 8000 Hz) with the helpers.
#   DOUBLE_Q=1 docs/tools/ppc_check/run.sh   also compares with Q passed as a double (exact decimal, eg 0.9)
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../../../components/tas58xx"
OUT="$(mktemp -d)"
g++ -std=gnu++20 -O2 -I"$HERE/../biquad_check/stub" -I"$SRC" "$HERE/cfgtest.cpp" "$SRC/tas58xx_helpers.cpp" -o "$OUT/cfgtest"
echo "== helpers as in the component (Q is a float)"
python3 "$HERE/compare.py" "$OUT/cfgtest"
if [ "${DOUBLE_Q:-0}" = "1" ]; then
  mkdir "$OUT/dq" && cp "$SRC/tas58xx_helpers.cpp" "$SRC/tas58xx_helpers.h" "$OUT/dq/"
  sed -i 's/float q_factor/double q_factor/g' "$OUT/dq/tas58xx_helpers.cpp" "$OUT/dq/tas58xx_helpers.h"
  sed 's/float q = strtof(argv\[4\], nullptr);/double q = strtod(argv[4], nullptr);/' "$HERE/cfgtest.cpp" > "$OUT/dq/cfgtest.cpp"
  g++ -std=gnu++20 -O2 -I"$HERE/../biquad_check/stub" -I"$OUT/dq" "$OUT/dq/cfgtest.cpp" "$OUT/dq/tas58xx_helpers.cpp" -o "$OUT/cfgtest_dq"
  echo "== Q passed as a double"
  python3 "$HERE/compare.py" "$OUT/cfgtest_dq"
fi
rm -rf "$OUT"
