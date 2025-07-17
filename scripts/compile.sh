#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."
CXX=${CXX:-g++}
if ! command -v "$CXX" >/dev/null; then
    if command -v clang++ >/dev/null; then
        CXX=clang++
    else
        echo "No C++ compiler found" >&2
        exit 1
    fi
fi
PKG_CFLAGS="$(pkg-config --cflags libgit2 2>/dev/null || echo '') $(pkg-config --cflags yaml-cpp 2>/dev/null || echo '')"
PKG_LIBS="$(pkg-config --libs libgit2 2>/dev/null || echo '-lgit2') $(pkg-config --libs yaml-cpp 2>/dev/null || echo '-lyaml-cpp')"
mkdir -p dist
$CXX -std=c++20 -Iinclude $PKG_CFLAGS \
    src/autogitpull.cpp src/git_utils.cpp src/tui.cpp src/logger.cpp \
    src/resource_utils.cpp src/system_utils.cpp src/time_utils.cpp \
    src/config_utils.cpp src/debug_utils.cpp src/parse_utils.cpp $PKG_LIBS -o dist/autogitpull
