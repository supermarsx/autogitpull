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
