#!/usr/bin/env bash
set -e
echo "Compiling debug build..."
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
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

mkdir -p "${ROOT_DIR}/dist"
$CXX -std=c++20 -O0 -g -fsanitize=address -DYAML_CPP_STATIC_DEFINE -I"${ROOT_DIR}/include" $PKG_CFLAGS \
    "${ROOT_DIR}/src/autogitpull.cpp" "${ROOT_DIR}/src/git_utils.cpp" "${ROOT_DIR}/src/tui.cpp" "${ROOT_DIR}/src/logger.cpp" \
    "${ROOT_DIR}/src/resource_utils.cpp" "${ROOT_DIR}/src/system_utils.cpp" "${ROOT_DIR}/src/time_utils.cpp" \
    "${ROOT_DIR}/src/config_utils.cpp" "${ROOT_DIR}/src/debug_utils.cpp" "${ROOT_DIR}/src/options.cpp" \
    "${ROOT_DIR}/src/parse_utils.cpp" "${ROOT_DIR}/src/lock_utils.cpp" "${ROOT_DIR}/src/linux_daemon.cpp" $PKG_LIBS \
    -fsanitize=address -o "${ROOT_DIR}/dist/autogitpull_debug"
echo "Build complete: ${ROOT_DIR}/dist/autogitpull_debug"

