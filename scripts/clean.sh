#!/usr/bin/env bash
set -e
echo "Cleaning build artifacts..."
rm -f autogitpull autogitpull.exe
rm -f *.o *.obj
if [ -d build ]; then
  echo "Removing build directory..."
  rm -rf build
fi
if [ -d dist ]; then
  echo "Removing dist directory..."
  rm -rf dist
fi
echo "Clean complete."
