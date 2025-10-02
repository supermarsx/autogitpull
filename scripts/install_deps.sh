#!/usr/bin/env bash
set -euo pipefail

echo "[autogitpull] Dependency install script is deprecated."
echo "CMake (FetchContent) now builds third-party libs automatically."

declare -a missing
for tool in cmake git; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    missing+=("$tool")
  fi
done

if ((${#missing[@]})); then
  echo "Missing required tools: ${missing[*]}" >&2
  echo "Please install the tools above and ensure they are on your PATH." >&2
  exit 1
fi

echo "You're good to go: run"
echo "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build -j"
