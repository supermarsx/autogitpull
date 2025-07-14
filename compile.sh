#!/usr/bin/env bash
set -e

os="$(uname -s)"

if command -v pkg-config >/dev/null && pkg-config --exists libgit2; then
    :
else
    if [[ "$os" == "Darwin" ]]; then
        if command -v brew >/dev/null && brew ls --versions libgit2 >/dev/null; then
            echo "libgit2 found via brew"
        else
            echo "libgit2 not found, attempting to install..."
            ./install_deps.sh
        fi
    else
        echo "libgit2 not found, attempting to install..."
        ./install_deps.sh
    fi
fi

g++ -std=c++17 autogitpull.cpp git_utils.cpp tui.cpp -lgit2 -o git_auto_pull_all


