#!/usr/bin/env bash
set -euo pipefail

cmake -B build -G Xcode
cmake --build build --config Debug 2>&1 | grep -E "Undefined symbols|^ld:|warning:|error:|BUILD (FAILED|SUCCEEDED)" -A5
ctest --test-dir build -C Debug --output-on-failure