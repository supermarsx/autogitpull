#!/usr/bin/env bash
set -e

os="$(uname -s)"

if [[ "$os" == "Darwin" ]]; then
    if ! command -v clang++ >/dev/null; then
        echo "clang++ not found, attempting to install..."
        xcode-select --install || true
    fi
else
    if ! command -v g++ >/dev/null; then
        echo "g++ not found, attempting to install..."
        if command -v apt-get >/dev/null; then
            sudo apt-get update && sudo apt-get install -y g++
        elif command -v yum >/dev/null; then
            sudo yum install -y gcc-c++
        fi
    fi
fi

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

PKG_CFLAGS="$(pkg-config --cflags libgit2 2>/dev/null || echo '')"
if [[ "$os" == "Darwin" ]]; then
    PKG_LIBS="$(pkg-config --libs libgit2 2>/dev/null || echo '-lgit2')"
    g++ -std=c++17 ${PKG_CFLAGS} autogitpull.cpp git_utils.cpp tui.cpp ${PKG_LIBS} -o autogitpull
else
    PKG_LIBS="$(pkg-config --static --libs libgit2 2>/dev/null || echo '-lgit2')"
    g++ -std=c++17 -static ${PKG_CFLAGS} autogitpull.cpp git_utils.cpp tui.cpp ${PKG_LIBS} -o autogitpull
fi



