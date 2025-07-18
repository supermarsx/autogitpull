#!/usr/bin/env bash
set -e
echo "Installing dependencies..."


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


# Check if libgit2, yaml-cpp and nlohmann-json are installed
libgit2_installed=false
yamlcpp_installed=false
json_installed=false

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
fi

if $libgit2_installed && $yamlcpp_installed && $json_installed; then
    echo "libgit2, yaml-cpp and nlohmann-json already installed"
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
            sudo apt-get update
            sudo apt-get install -y "${packages[@]}"
        elif command -v yum >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2-devel)
            $yamlcpp_installed || packages+=(yaml-cpp-devel)
            $json_installed || packages+=(nlohmann-json-devel)
            sudo yum install -y "${packages[@]}"
        elif command -v zypper >/dev/null; then
            packages=()
            $libgit2_installed || packages+=(libgit2-devel)
            $yamlcpp_installed || packages+=(yaml-cpp-devel)
            $json_installed || packages+=(nlohmann_json-devel)
            sudo zypper install -y "${packages[@]}"
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
            brew install "${packages[@]}"
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
