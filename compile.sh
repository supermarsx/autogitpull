#!/usr/bin/env bash
set -e

if ! pkg-config --exists libgit2; then
    echo "libgit2 not found, attempting to install..."
    ./install_deps.sh
fi

g++ -std=c++17 autogitpull.cpp git_utils.cpp tui.cpp -lgit2 -o git_auto_pull_all


