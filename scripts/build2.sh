#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

if [ ! -f "${ROOT_DIR}/graphics/icon.ico" ] || [ ! -f "${ROOT_DIR}/graphics/icon.icns" ]; then
    "${SCRIPT_DIR}/generate_icons.sh"
fi

cd "${ROOT_DIR}"

if [ ! -f build/config.build ] ; then
    b configure
fi
b install config.install.root="${ROOT_DIR}/dist"
