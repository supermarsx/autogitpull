#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

# Ensure build2 can locate zlib installed by Homebrew on macOS.
if [[ "$(uname -s)" == "Darwin" ]] && command -v brew >/dev/null; then
    ZLIB_PKG="$(brew --prefix)/opt/zlib/lib/pkgconfig"
    if [ -d "$ZLIB_PKG" ]; then
        export PKG_CONFIG_PATH="$ZLIB_PKG:${PKG_CONFIG_PATH:-}"
    fi
fi

if [ ! -f "${ROOT_DIR}/graphics/icon.ico" ] || [ ! -f "${ROOT_DIR}/graphics/icon.icns" ]; then
    "${SCRIPT_DIR}/generate_icons.sh"
fi

cd "${ROOT_DIR}"

if [ ! -f build/config.build ] ; then
    b configure
fi
b install config.install.root="${ROOT_DIR}/dist"
