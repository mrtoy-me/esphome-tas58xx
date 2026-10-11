#!/usr/bin/env bash
# Parses every tas58xx .cpp with clang-tidy against ESPHome headers, for several define sets.
# Usage: docs/tools/cpp_check.sh <esphome-clone> [arduinojson-src]
#   <esphome-clone>   checkout of github.com/esphome/esphome (beta or dev); its
#                     esphome/core/defines.h is edited (USE_LVGL lines removed) and
#                     esphome/components/tas58xx is replaced, so use a scratch clone
#   [arduinojson-src] ArduinoJson src dir (git clone -b v7.4.2 https://github.com/bblanchon/ArduinoJson)
# A result is only meaningful with fatal-include-errors=0: a fatal error (eg lvgl.h not found)
# stops clang before the function bodies are checked.
set -u
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
ESPHOME="$(cd "$1" && pwd)"
AJ="${2:-$ESPHOME/../ArduinoJson/src}"
DEST="$ESPHOME/esphome/components/tas58xx"

sed -i '/USE_LVGL/d' "$ESPHOME/esphome/core/defines.h"
rm -rf "$DEST" && mkdir "$DEST" && cp -r "$REPO/components/tas58xx/." "$DEST/" && rm -rf "$DEST/Example YAML"

cd "$ESPHOME"
for combo in "-DUSE_TAS5805M_DAC" \
             "-DUSE_TAS5805M_DAC -DUSE_TAS58XX_EQ_GAINS -DUSE_TAS58XX_CHANNEL_VOLUMES -DUSE_TAS58XX_BINARY_SENSOR" \
             "-DUSE_TAS5805M_DAC -DUSE_TAS58XX_EQ_GAINS -DUSE_TAS58XX_EQ_BIAMP" \
             "-DUSE_TAS5825M_DAC -DUSE_TAS58XX_EQ_PRESETS -DUSE_TAS58XX_CHANNEL_VOLUMES" \
             "-DUSE_TAS5825M_DAC -DUSE_TAS58XX_EQ_GAINS -DUSE_TAS58XX_EQ_BIAMP -DUSE_TAS58XX_BINARY_SENSOR"; do
  all=""
  for f in $(find esphome/components/tas58xx -name '*.cpp'); do
    all+=$(clang-tidy --checks=-*,llvm-namespace-comment "$f" -- -std=gnu++20 -I. -I"$AJ" -DUSE_HOST $combo 2>&1)$'\n'
  done
  ours=$(echo "$all" | grep -E 'components/tas58xx/.*(error|warning):' | sort -u)
  echo "[$combo] tas58xx diagnostics=$(echo -n "$ours" | grep -c .) fatal-include-errors=$(echo "$all" | grep -c 'file not found')"
  echo "$ours" | grep . | head -5
done
