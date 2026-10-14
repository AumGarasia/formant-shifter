#!/usr/bin/env bash
set -euo pipefail

PV="${PLUGINVAL:-/Applications/pluginval.app/Contents/MacOS/pluginval}"
LEVEL="${1:-5}"
LOGS=build/pluginval-logs
mkdir -p "$LOGS"

for plugin in "$HOME/Library/Audio/Plug-Ins/VST3/Formant Shifter.vst3" \
              "$HOME/Library/Audio/Plug-Ins/Components/Formant Shifter.component"; do
    echo "== pluginval level $LEVEL: $plugin"
    "$PV" --strictness-level "$LEVEL" --output-dir "$LOGS" "$plugin" | tail -15
done