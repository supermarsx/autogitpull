# autogitpull
Automatic Git Puller & Monitor

## Dependencies
This tool relies on [libgit2](https://libgit2.org/). The helper scripts
`install_deps.sh` (Linux/macOS) and `install_deps.bat` (Windows) automatically
download and install `libgit2` when needed.  The project links against the
static version of the library so the final executable does not depend on a
separate `libgit2` DLL.  If you prefer to install the library yourself, follow
the instructions below.

### Installing libgit2 on Linux
```
sudo apt-get update && sudo apt-get install -y libgit2-dev     # Debian/Ubuntu
sudo yum install -y libgit2-devel                              # RHEL/Fedora
```

### Installing libgit2 on Windows

#### MSVC (Visual Studio)
Use [vcpkg](https://github.com/microsoft/vcpkg):

```
git clone https://github.com/microsoft/vcpkg
cd vcpkg && bootstrap-vcpkg.bat
vcpkg\vcpkg install libgit2
```

Ensure the resulting `installed` folder is on your `LIB` and `INCLUDE`
paths when compiling with `compile-cl.bat`.

#### MinGW
Run `install_libgit2_mingw.bat` to build libgit2 natively with MinGW. The
script installs the static library and headers under `libgit2\_install`.

`compile-cl.bat` expects a vcpkg installation while `compile.bat` uses the
library produced by `install_libgit2_mingw.bat` and will call it
automatically if `libgit2\_install` is missing. When linking with MinGW,
additional Windows system libraries are required. `compile.bat` now attempts
to install MinGW through Chocolatey if `g++` is not found and already
includes `winhttp`, `ole32`, `rpcrt4` and `crypt32` so that the build
succeeds without manual tweaks.

## Building
### Using the provided scripts
Run `make` (Linux/macOS), `compile.bat` (MinGW) or `compile-cl.bat` (MSVC) to
build the project. `compile.bat` invokes `install_libgit2_mingw.bat` when
`libgit2` is missing. The binary is produced as `autogitpull` (or
`autogitpull.exe` on Windows).

The repository also ships with `compile.sh` for Unix-like environments which
will attempt to install a C++ compiler if one isn't present. Windows users get
`compile.bat` (MinGW) and `compile-cl.bat` (MSVC) along with
`install_deps.bat`, `install_libgit2_mingw.bat` and `run.bat`.

Clean up intermediate files with `make clean`.

### Manual compilation
If you prefer to build without the helper scripts, the following commands show
the bare minimum required to compile the program.

On Linux with `g++`:

```bash
g++ -std=c++17 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp $(pkg-config --cflags --libs libgit2) -pthread -static -o autogitpull
```

On macOS with `clang++`:

```bash
clang++ -std=c++17 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp $(pkg-config --cflags --libs libgit2) -pthread -o autogitpull
```

On Windows with MSVC's `cl`:

```batch
cl /std:c++17 /EHsc /MT /Ipath\to\libgit2\include autogitpull.cpp git_utils.cpp tui.cpp logger.cpp /link /LIBPATH:path\to\libgit2\lib git2.lib
```

These commands mirror what the scripts do internally.

### Building with CMake
Alternatively, configure the project with CMake:

```bash
cmake -S . -B build
cmake --build build
```

The resulting executable will be in the `build` directory.

### Running tests
Unit tests require [Catch2](https://github.com/catchorg/Catch2), which is
available on most package managers. After installing the dependency, configure
and build the project with CMake and run `ctest`:

```bash
make test
```

This command generates a `build` directory (if missing), compiles the tests and
executes them through CMake's `ctest` driver.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--log-file <path>] [--concurrency <n>] [--check-only] [--no-hash-check] [--help]`

Available options:

* `--include-private` – include private or non-GitHub repositories in the scan.
* `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
* `--interval <seconds>` – delay between automatic scans (default 30).
* `--refresh-rate <ms>` – how often the TUI refreshes in milliseconds (default 250).
* `--log-dir <path>` – directory where pull logs will be written.
* `--log-file <path>` – file for general messages.
* `--concurrency <n>` – number of repositories processed in parallel (default 3).
* `--check-only` – only check for updates without pulling.
* `--no-hash-check` – always pull without comparing commit hashes first.
* `--help` – show the usage information and exit.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log
is written to a timestamped file inside this directory and its location is shown in the TUI.
Use `--log-file <path>` to append high level messages to the given file. The program records startup, repository actions and shutdown there. For example:
`./autogitpull myprojects --log-dir logs --log-file autogitpull.log`

## Runtime requirements
* **Git** must be available in your `PATH` for libgit2 to interact with repositories.
* Network access is required to contact remote Git servers when pulling updates.
* The application prints ANSI color codes; on Windows run it in a terminal that
  supports color (e.g. Windows Terminal or recent PowerShell).

## Licensing
autogitpull is licensed under the MIT license (see `LICENSE`). The project
bundles the license for the statically linked libgit2 library in
`LIBGIT2_LICENSE`.
