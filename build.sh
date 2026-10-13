#!/usr/bin/env bash
set -euo pipefail

cmake -B build -G Xcode
cmake --build build --config Debug --target fsdsp_tests offline_render 2>&1 \
  | grep -E "Undefined symbols|^ld:|error:|BUILD (FAILED|SUCCEEDED)|/(src|tests|tools)/.*warning:" -A5
ctest --test-dir build -C Debug --output-on-failure