#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Release}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found"
  exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "ninja not found"
  exit 1
fi

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DJUCE_PATH="$(realpath ../JUCE)"
cmake --build build --parallel
