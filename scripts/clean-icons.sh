#!/usr/bin/env bash
# ---------------------------------------------
# cleanup_icons.sh – remove generated icon files
# ---------------------------------------------
set -euo pipefail

# Move to project root (assumes the script lives in scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR%/*}"
GRAPHICS_DIR="$PROJECT_ROOT/graphics"

printf '\n==========================================\n'
printf 'Cleaning up generated icons...\n'
printf '==========================================\n\n'

if [[ ! -d "$GRAPHICS_DIR" ]]; then
  echo "[Error] graphics directory not found: $GRAPHICS_DIR" >&2
  exit 1
fi

cd "$GRAPHICS_DIR"

echo "[Step] Removing .ico and .icns files"
rm -f icon.ico icon.icns

echo "[Step] Removing PNG variants"
for sz in 16 32 48 128 256 512 1024; do
  rm -f "icon_${sz}.png"
  rm -f "icon_${sz}x${sz}.png" 2>/dev/null || true
done

echo "[Step] Removing icon.iconset directory (if exists)"
rm -rf icon.iconset

printf '\n==========================================\n'
printf 'Cleanup complete!\n'
printf '====================