#!/usr/bin/env bash
set -euo pipefail

PV="${PLUGINVAL:-/Applications/pluginval.app/Contents/MacOS/pluginval}"
VALIDATOR="${VST3_VALIDATOR:-$HOME/dev/vst3sdk/cmake-build/bin/Release/validator}"
LEVEL="${1:-5}"
LOGS=build/pluginval-logs
mkdir -p "$LOGS"

for plugin in "$HOME/Library/Audio/Plug-Ins/VST3/Formant Shifter.vst3" \
              "$HOME/Library/Audio/Plug-Ins/Components/Formant Shifter.component"; do
    echo "== pluginval level $LEVEL: $plugin"

    if [[ -x "$VALIDATOR" ]]; then
        "$PV" --strictness-level "$LEVEL" --vst3validator "$VALIDATOR" --output-dir "$LOGS" "$plugin" | tail -15
    else
        echo "(Steinberg VST3 validator not found at $VALIDATOR: skipping that step)"
        "$PV" --strictness-level "$LEVEL" --output-dir "$LOGS" "$plugin" | tail -15
    fi
done