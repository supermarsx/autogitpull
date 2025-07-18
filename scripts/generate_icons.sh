#!/usr/bin/env bash
set -e

# Generate platform icons from graphics/icon.png
SRC="$(dirname "$0")/../graphics/icon.png"
DST_DIR="$(dirname "$0")/../graphics"

# Ensure ImageMagick's convert is available
if ! command -v convert >/dev/null; then
    echo "ImageMagick not found. Cloning and building from source..."
    TMPDIR=$(mktemp -d)
    git clone --depth 1 https://github.com/ImageMagick/ImageMagick "$TMPDIR/ImageMagick"
    pushd "$TMPDIR/ImageMagick" >/dev/null
    ./configure --prefix="$TMPDIR/im-install"
    make -j"$(nproc)"
    make install
    export PATH="$TMPDIR/im-install/bin:$PATH"
    popd >/dev/null
fi

mkdir -p "$DST_DIR"
cd "$DST_DIR"

# Windows ICO
if [ ! -f icon.ico ]; then
    convert "$SRC" -resize 16x16 icon_16.png
    convert "$SRC" -resize 32x32 icon_32.png
    convert "$SRC" -resize 48x48 icon_48.png
    convert "$SRC" -resize 256x256 icon_256.png
    convert icon_16.png icon_32.png icon_48.png icon_256.png icon.ico
fi

# macOS ICNS
if [ ! -f icon.icns ]; then
    ICONSET=icon.iconset
    mkdir -p "$ICONSET"
    convert "$SRC" -resize 16x16 "$ICONSET/icon_16x16.png"
    convert "$SRC" -resize 32x32 "$ICONSET/icon_16x16@2x.png"
    convert "$SRC" -resize 32x32 "$ICONSET/icon_32x32.png"
    convert "$SRC" -resize 64x64 "$ICONSET/icon_32x32@2x.png"
    convert "$SRC" -resize 128x128 "$ICONSET/icon_128x128.png"
    convert "$SRC" -resize 256x256 "$ICONSET/icon_128x128@2x.png"
    convert "$SRC" -resize 256x256 "$ICONSET/icon_256x256.png"
    convert "$SRC" -resize 512x512 "$ICONSET/icon_256x256@2x.png"
    convert "$SRC" -resize 512x512 "$ICONSET/icon_512x512.png"
    convert "$SRC" -resize 1024x1024 "$ICONSET/icon_512x512@2x.png"
    if command -v iconutil >/dev/null; then
        iconutil -c icns "$ICONSET" -o icon.icns
    elif command -v png2icns >/dev/null; then
        png2icns icon.icns "$ICONSET"/icon_16x16.png "$ICONSET"/icon_32x32.png \
            "$ICONSET"/icon_128x128.png "$ICONSET"/icon_256x256.png \
            "$ICONSET"/icon_512x512.png
    else
        echo "Neither iconutil nor png2icns found; skipping ICNS generation" >&2
    fi
fi

# Linux PNGs
for sz in 16 32 48 128 256; do
    if [ ! -f "icon_${sz}.png" ]; then
        convert "$SRC" -resize ${sz}x${sz} "icon_${sz}.png"
    fi
done

echo "Icons generated in $DST_DIR"
