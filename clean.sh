#!/usr/bin/env bash
set -e
rm -f autogitpull autogitpull.exe
rm -f *.o *.obj
if [ -d build ]; then
  rm -rf build
fi
