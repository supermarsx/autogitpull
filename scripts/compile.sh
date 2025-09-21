#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo " AutoGitPull - Release Build"
echo "============================================================"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

###############################################################################
# 1. Locate compiler
###############################################################################
echo "[1/4] Checking for C++ compiler..."
CXX=${CXX:-g++}
if ! command -v "$CXX" >/dev/null 2>&1; then
    if command -v clang++ >/dev/null 2>&1; then
        CXX=clang++
        echo "    g++ not found, using clang++"
    else
        echo "    No C++ compiler found" >&2
        exit 1
    fi
else
    echo "    Using $CXX"
fi

###############################################################################
# 2. Resolve dependency flags
###############################################################################
echo "[2/4] Resolving dependencies..."
PKG_CFLAGS="$(pkg-config --cflags libgit2 2>/dev/null || echo '') $(pkg-config --cflags yaml-cpp 2>/dev/null || echo '')"
PKG_LIBS="$(pkg-config --libs libgit2 2>/dev/null || echo '-lgit2') $(pkg-config --libs yaml-cpp 2>/dev/null || echo '-lyaml-cpp')"

###############################################################################
# 3. Ensure resources
###############################################################################
if [[ ! -f "${ROOT_DIR}/graphics/icon.ico" ]] || [[ ! -f "${ROOT_DIR}/graphics/icon.icns" ]]; then
    echo "[3/4] Generating icons..."
    "${SCRIPT_DIR}/generate_icons.sh"
else
    echo "[3/4] Icons already generated"
fi

###############################################################################
# 4. Compile sources
###############################################################################
echo "[4/4] Compiling sources..."

SRCS=(
    "${ROOT_DIR}/src/autogitpull.cpp"
    "${ROOT_DIR}/src/scanner.cpp"
    "${ROOT_DIR}/src/ui_loop.cpp"
    "${ROOT_DIR}/src/file_watch.cpp"
    "${ROOT_DIR}/src/git_utils.cpp"
    "${ROOT_DIR}/src/tui.cpp"
    "${ROOT_DIR}/src/logger.cpp"
    "${ROOT_DIR}/src/resource_utils.cpp"
    "${ROOT_DIR}/src/system_utils.cpp"
    "${ROOT_DIR}/src/time_utils.cpp"
    "${ROOT_DIR}/src/config_utils.cpp"
    "${ROOT_DIR}/src/ignore_utils.cpp"
    "${ROOT_DIR}/src/debug_utils.cpp"
    "${ROOT_DIR}/src/options.cpp"
    "${ROOT_DIR}/src/parse_utils.cpp"
    "${ROOT_DIR}/src/history_utils.cpp"
    "${ROOT_DIR}/src/mutant_mode.cpp"
    "${ROOT_DIR}/src/lock_utils_posix.cpp"
    "${ROOT_DIR}/src/process_monitor.cpp"
    "${ROOT_DIR}/src/help_text.cpp"
    "${ROOT_DIR}/src/cli_commands.cpp"
    "${ROOT_DIR}/src/linux_daemon.cpp"
    "${ROOT_DIR}/src/windows_service.cpp"
    "${ROOT_DIR}/src/linux_commands.cpp"
    "${ROOT_DIR}/src/windows_commands.cpp"
)

mkdir -p "${ROOT_DIR}/dist"
"$CXX" -std=c++20 -O2 -DNDEBUG -DYAML_CPP_STATIC_DEFINE \
    -I "${ROOT_DIR}/include" ${PKG_CFLAGS} "${SRCS[@]}" ${PKG_LIBS} \
    -o "${ROOT_DIR}/dist/autogitpull"

echo "Build complete: ${ROOT_DIR}/dist/autogitpull"
