# Contribution Guidelines

To maintain code quality, all commits must pass the following checks **before** they are committed:

1. `make lint` – runs `clang-format` and `cpplint` using the project's `.clang-format` configuration.
2. The full test suite via `make test`.

Run these commands locally from the repository root:

```bash
make lint
make test
```

`clang-format` (configured by `.clang-format`) and `cpplint` are mandatory tools. You can auto-format sources with:

```bash
clang-format -i <file.cpp> <file.hpp>
```

If any command fails, fix the issues before committing. Pull requests failing any of these checks will be rejected.

When adding new C++ source files, update all build scripts (`compile.sh`, `compile.bat`, and `compile-cl.bat`) so they compile the new files. Build script changes are mandatory.
## Repository Overview
The codebase is organized into several directories:
- `src/` — C++ source files implementing the application.
  - `autogitpull.cpp` is the CLI entry point that drives scanning and pulling.
  - `git_utils.cpp` wraps libgit2 interactions.
  - `tui.cpp` implements the optional text UI.
  - `config_utils.cpp` parses YAML/JSON configs.
  - `logger.cpp` provides logging helpers.
  - `resource_utils.cpp` tracks CPU and memory usage.
  - `system_utils.cpp` exposes system helpers.
  - `time_utils.cpp` contains timing utilities.
  - `debug_utils.cpp` holds debug helpers.
  - `options.cpp` defines command line options.
  - `parse_utils.cpp` assists with CLI parsing.
- `include/` — Header files matching the sources.
- `tests/` — Catch2-based unit tests.
- `examples/` — Sample configuration files.
- `scripts/` — Build and dependency scripts.
- `dist/` — Created by the build; holds binaries.
