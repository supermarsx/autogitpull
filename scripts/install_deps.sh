#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

echo "Installing dependencies..."

# Resolve repo root (for local venv tools if needed)
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

ensure_path() {
  case ":$PATH:" in
    *":$1:"*) ;;
    *) export PATH="$1:$PATH" ;;
  esac
}

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

install_cpplint() {
  if command -v cpplint >/dev/null 2>&1; then
    return 0
  fi

  case "$(uname -s)" in
    Darwin*)
      if command -v brew >/dev/null 2>&1; then
        echo "Installing cpplint via Homebrew..."
        if brew install cpplint; then
          return 0
        fi
      fi
      ;;
  esac

  if command -v pipx >/dev/null 2>&1; then
    echo "Installing cpplint via pipx..."
    if pipx install --include-deps cpplint; then
      # pipx installs shims into ~/.local/bin by default
      ensure_path "$HOME/.local/bin"
      return 0
    fi
  fi

  # Fallback: create a local venv under .tools and install cpplint there
  echo "Setting up local venv for cpplint (no system pip changes)..."
  TOOLS_DIR="$REPO_ROOT/.tools"
  VENV_DIR="$TOOLS_DIR/venv"
  mkdir -p "$TOOLS_DIR"
  if command -v python3 >/dev/null 2>&1; then
    python3 -m venv "$VENV_DIR"
  else
    echo "python3 not found. Cannot create virtual environment for cpplint." >&2
    return 1
  fi
  # shellcheck disable=SC1090
  source "$VENV_DIR/bin/activate"
  python -m pip install --upgrade pip >/dev/null 2>&1 || true
  python -m pip install cpplint
  # Prefer using the venv bin on PATH for this session
  ensure_path "$VENV_DIR/bin"
}

# Install cpplint if missing (without breaking externally-managed Python envs)
install_cpplint || echo "Warning: cpplint installation failed; ensure it is available before committing." >&2

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
