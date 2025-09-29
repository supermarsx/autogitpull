#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

echo "Installing dependencies..."

# Auto-detect make and cmake if missing from PATH
for tool in make cmake; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        for dir in /usr/local/opt/cmake/bin /usr/local/bin /opt/homebrew/bin /usr/bin; do
            if [ -x "$dir/$tool" ]; then
                export PATH="$dir:$PATH"
                break
            fi
        done
        if ! command -v "$tool" >/dev/null 2>&1; then
            echo "$tool not found. Please install $tool or add it to PATH." >&2
        fi
    fi
done

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

# Check if libgit2, yaml-cpp, nlohmann-json and zlib are installed
libgit2_installed=false
yamlcpp_installed=false
json_installed=false
zlib_installed=false

if command -v pkg-config >/dev/null; then
    if pkg-config --exists libgit2; then
        libgit2_installed=true
    fi
    if pkg-config --exists yaml-cpp; then
        yamlcpp_installed=true
    fi
    if pkg-config --exists nlohmann_json; then
        json_installed=true
    fi
    if pkg-config --exists zlib; then
        zlib_installed=true
    fi
fi

if $libgit2_installed && $yamlcpp_installed && $json_installed && $zlib_installed; then
    echo "libgit2, yaml-cpp, nlohmann-json and zlib already installed"
    exit 0
fi

os="$(uname -s)"
case "$os" in
    Linux*)
        if command -v apt-get >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2-dev)
            $yamlcpp_installed || packages+=(libyaml-cpp-dev)
            $json_installed || packages+=(nlohmann-json3-dev)
            $zlib_installed || packages+=(zlib1g-dev)
            if [ ${#packages[@]} -gt 0 ]; then
                sudo apt-get update
                sudo apt-get install -y "${packages[@]}"
            fi
        elif command -v yum >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2-devel)
            $yamlcpp_installed || packages+=(yaml-cpp-devel)
            $json_installed || packages+=(nlohmann-json-devel)
            $zlib_installed || packages+=(zlib-devel)
            if [ ${#packages[@]} -gt 0 ]; then
                sudo yum install -y "${packages[@]}"
            fi
        elif command -v zypper >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2-devel)
            $yamlcpp_installed || packages+=(yaml-cpp-devel)
            $json_installed || packages+=(nlohmann_json-devel)
            $zlib_installed || packages+=(zlib-devel)
            if [ ${#packages[@]} -gt 0 ]; then
                sudo zypper install -y "${packages[@]}"
            fi
        else
            echo "Unsupported Linux distribution. Please install libgit2, yaml-cpp, and nlohmann-json manually."
            exit 1
        fi
        ;;
    Darwin*)
        if command -v brew >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2)
            $yamlcpp_installed || packages+=(yaml-cpp)
            $json_installed || packages+=(nlohmann-json)
            $zlib_installed || packages+=(zlib)
            if [ ${#packages[@]} -gt 0 ]; then
                brew install "${packages[@]}"
            fi
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

