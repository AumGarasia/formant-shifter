#!/usr/bin/env bash
set -euo pipefail

cmake -B build -G Xcode
cmake --build build --config Release --target FormantShifter_AU FormantShifter_VST3 2>&1 \
  | grep -E "error:|BUILD (FAILED|SUCCEEDED)" -A3
ls -ld ~/Library/Audio/Plug-Ins/VST3/*Formant* ~/Library/Audio/Plug-Ins/Components/*Formant*
auval -v aufx Fmsh Yrco 2>&1 | grep -E "Time to open|VALIDATION"