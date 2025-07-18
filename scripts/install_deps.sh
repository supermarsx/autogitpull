#!/usr/bin/env bash
set -e


# Install cpplint if missing
if ! command -v cpplint >/dev/null; then
    if command -v pip3 >/dev/null; then
        pip3 install --user cpplint
        export PATH="$HOME/.local/bin:$PATH"
    elif command -v pip >/dev/null; then
        pip install --user cpplint
        export PATH="$HOME/.local/bin:$PATH"
    else
        echo "pip not found. Please install cpplint manually." >&2
    fi
fi

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
            sudo apt-get install -y libgit2-dev libyaml-cpp-dev nlohmann-json3-dev
        elif command -v yum >/dev/null; then
            sudo yum install -y libgit2-devel yaml-cpp-devel nlohmann-json-devel
        elif command -v zypper >/dev/null; then
            sudo zypper install -y libgit2-devel yaml-cpp-devel nlohmann_json-devel
        else
            echo "Unsupported Linux distribution. Please install libgit2, yaml-cpp, and nlohmann-json manually."
            exit 1
        fi
        ;;
    Darwin*)
        if command -v brew >/dev/null; then
            brew install libgit2 yaml-cpp nlohmann-json
        else
            echo "Homebrew not found. Please install libgit2, yaml-cpp, and nlohmann-json manually."
            exit 1
        fi
        ;;
    *)
        echo "Unsupported OS."
        exit 1
        ;;
esac
