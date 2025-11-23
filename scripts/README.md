# Scripts

This directory contains convenience and legacy scripts for common developer workflows. The preferred and supported build path is the CMake configuration + the cross-platform Python wrapper `scripts/build.py`.

Recommended commands (cross-platform):

- Configure and build (Release):

  python3 scripts/build.py --config Release --build-dir build

- Configure and build (Debug):

  python3 scripts/build.py --config Debug --build-dir build-debug

- Run tests (after building):

  python3 scripts/build.py --test --build-dir build

Notes:

- Many of the older `compile.*` and `install_*_mingw.*` scripts are retained as legacy fallbacks for specific environments (MinGW/MSVC). These scripts are marked with a deprecation note and will prefer `scripts/build.py` if Python is available.

- Dependency management should be done via CMake FetchContent. `install_deps.*` files are only present for rare bootstrapping scenarios and are not the recommended path.

- Use `scripts/generate_icons.*` to regenerate OS-specific icons from `graphics/icon.png`.

- Use `scripts/gen_docs.py` to regenerate `docs/cli_options.md` and `examples/example-config.*` from internal help text. It can be run from the repo root or `scripts/` directory.

- Verify scripts health with the simple checker:

  python3 scripts/check-scripts.py

- `clang_format.cmake` and `run_cpplint.cmake` are CMake helpers for CI and can be invoked from the main CMake project.
