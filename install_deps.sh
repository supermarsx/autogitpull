#!/usr/bin/env bash
set -e

# Install libgit2 if missing
if command -v pkg-config >/dev/null && pkg-config --exists libgit2; then
    echo "libgit2 already installed"
    exit 0
fi

os="$(uname -s)"
case "$os" in
    Linux*)
        if command -v apt-get >/dev/null; then
            sudo apt-get update
            sudo apt-get install -y libgit2-dev libyaml-cpp-dev
        elif command -v yum >/dev/null; then
            sudo yum install -y libgit2-devel yaml-cpp-devel
        else
            echo "Unsupported Linux distribution. Please install libgit2 manually."
            exit 1
        fi
        ;;
    Darwin*)
        if command -v brew >/dev/null; then
            brew install libgit2
        else
            echo "Homebrew not found. Please install libgit2 manually."
            exit 1
        fi
        ;;
    *)
        echo "Unsupported OS."
        exit 1
        ;;
esac
