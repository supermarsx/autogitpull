#!/usr/bin/env bash
set -euo pipefail

echo "[autogitpull] Dependency install script is deprecated."
echo "CMake (FetchContent) now builds third-party libs automatically."

missing=()
for tool in cmake git; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    missing+=("$tool")
  fi
done

if ((${#missing[@]})); then
  echo "Missing required tools: ${missing[*]}" >&2
  echo "Please install the tools above and ensure they are on your PATH." >&2
  #!/usr/bin/env bash
  set -euo pipefail

  cat <<'EOF'
  DEPRECATED: install_deps.sh

  This repository now uses CMake FetchContent to obtain and build
  third-party libraries. In most situations you should configure and
  build the project using CMake directly or the convenience wrapper in
  scripts/build.py.

  Examples:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 8

  If you need to perform manual installs for an older workflow, see the
  legacy installers in scripts/ (install_libgit2_mingw.bat, etc.).
  EOF

  exit 0
