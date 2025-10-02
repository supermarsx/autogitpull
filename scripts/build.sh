#!/usr/bin/env bash
set -euo pipefail

# Cross-platform (POSIX shell) wrapper for CMake builds.
# Prefers delegating to scripts/build.py if Python is available.

CONFIG="${CONFIG:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
GENERATOR="${CMAKE_GENERATOR:-}"
PARALLEL="${PARALLEL:-0}"
TARGET=""
DO_TEST=0
DO_INSTALL=0
PREFIX="${PREFIX:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config) CONFIG="$2"; shift 2;;
    --build-dir) BUILD_DIR="$2"; shift 2;;
    -G|--generator) GENERATOR="$2"; shift 2;;
    -j|--parallel) PARALLEL="$2"; shift 2;;
    --target) TARGET="$2"; shift 2;;
    --test) DO_TEST=1; shift;;
    --install) DO_INSTALL=1; shift;;
    --prefix) PREFIX="$2"; shift 2;;
    -h|--help)
      echo "Usage: $0 [--config <cfg>] [--build-dir <dir>] [-G <gen>] [-j <N>] [--target <tgt>] [--test] [--install] [--prefix <dir>]";
      exit 0;;
    *) echo "Unknown argument: $1"; exit 2;;
  esac
done

PY_CMD=""
if command -v python3 >/dev/null 2>&1; then
  PY_CMD=python3
elif command -v python >/dev/null 2>&1; then
  PY_CMD=python
fi

if [[ -n "$PY_CMD" ]]; then
  set -x
  "$PY_CMD" "$(dirname "$0")/build.py" \
    --config "$CONFIG" \
    --build-dir "$BUILD_DIR" \
    ${GENERATOR:+-G "$GENERATOR"} \
    ${PARALLEL:+-j "$PARALLEL"} \
    ${TARGET:+--target "$TARGET"} \
    $( ((DO_TEST)) && echo --test ) \
    $( ((DO_INSTALL)) && echo --install ) \
    ${PREFIX:+--prefix "$PREFIX"}
  exit $?
fi

# Fallback: call cmake/ctest directly (no custom logic)
set -x
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG" ${GENERATOR:+-G "$GENERATOR"}
cmake --build "$BUILD_DIR" --config "$CONFIG" ${PARALLEL:+-j "$PARALLEL"} ${TARGET:+--target "$TARGET"}
if (( DO_INSTALL )); then
  if [[ -n "$PREFIX" ]]; then
    cmake --install "$BUILD_DIR" --prefix "$PREFIX"
  else
    cmake --install "$BUILD_DIR"
  fi
fi
if (( DO_TEST )); then
  ctest --test-dir "$BUILD_DIR" --output-on-failure -C "$CONFIG"
fi

