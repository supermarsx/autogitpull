#autogitpull
Automatic Git Puller & Monitor

## Dependencies

This tool relies on [libgit2](https://libgit2.org/). The helper scripts
`install_deps.sh` (Linux/macOS) and `install_deps.bat` (Windows) automatically
download and install `libgit2` when needed. You can also run `make deps` on
Unix-like systems to invoke the installer. The build requires the development
package (named `libgit2-dev` on Debian/Ubuntu). The Makefile tries to link
statically but falls back to dynamic linking when static libraries are
unavailable. If you prefer to install the library yourself, follow the
instructions below.

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

### Debug builds for leak analysis

The scripts `compile-debug.sh`, `compile-debug.bat` and `compile-debug-cl.bat`
compile the program with AddressSanitizer and debug information enabled. They
produce `autogitpull_debug` (or `autogitpull_debug.exe` on Windows). Use these
builds when running leak detection tools:

```bash
./compile-debug.sh      # Linux/macOS
compile-debug.bat       # MinGW
compile-debug-cl.bat    # MSVC
```

### Manual compilation

If you prefer to build without the helper scripts, the following commands show
the bare minimum required to compile the program.

On Linux with `g++`:

```bash
g++ -std=c++20 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp \
    $(pkg-config --cflags libgit2) \
    $(pkg-config --static --libs libgit2 2>/dev/null || pkg-config --libs libgit2) \
    -pthread -o autogitpull
```

On macOS with `clang++`:

```bash
clang++ -std=c++20 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp $(pkg-config --cflags --libs libgit2) -pthread -o autogitpull
```

On Windows with MSVC's `cl`:

```batch
cl /std:c++20 /EHsc /MT /Ipath\to\libgit2\include autogitpull.cpp git_utils.cpp tui.cpp logger.cpp /link /LIBPATH:path\to\libgit2\lib git2.lib
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

Unit tests use [Catch2](https://github.com/catchorg/Catch2). If the library is
not installed, CMake will automatically download it using `FetchContent`. **Make
sure `libgit2` is installed** (run `./install_deps.sh` on Linux/macOS or
`install_deps.bat` on Windows) before configuring and building the tests.
Once the dependencies are in place, run `ctest`:

```bash
make test
```

This command generates a `build` directory (if missing), compiles the tests and
executes them through CMake's `ctest` driver.

### Leak test

To run the memory leak regression test you need both the `valgrind` tool and the
`libgit2-dev` package installed. After building the tests with `make test`, run:

```bash
valgrind ./build/memory_leak_test
```

Valgrind should finish with the message `All heap blocks were freed -- no leaks
are possible`.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--show-version] [--version] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--log-file <path>] [--log-level <level>] [--verbose] [--concurrency <n>] [--threads <n>] [--single-thread] [--max-threads <n>] [--cpu-percent <n>] [--cpu-cores <mask>] [--mem-limit <MB>] [--check-only] [--no-hash-check] [--no-cpu-tracker] [--no-mem-tracker] [--no-thread-tracker] [--net-tracker] [--cli] [--silent] [--help]`

Available options:

- `--include-private` – include private or non-GitHub repositories in the scan.
- `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
- `--show-version` – display the program version in the TUI header.
- `--version` – print the program version and exit.
- `--interval <seconds>` – delay between automatic scans (default 30).
- `--refresh-rate <ms>` – how often the TUI refreshes in milliseconds (default 250).
- `--log-dir <path>` – directory where pull logs will be written.
- `--log-file <path>` – file for general messages.
- `--log-level <level>` – minimum message level written to the log (`DEBUG`, `INFO`, `WARNING`, `ERROR`).
- `--verbose` – shorthand for `--log-level DEBUG`.
- `--concurrency <n>` – number of repositories processed in parallel (default: hardware concurrency).
- `--threads <n>` – alias for `--concurrency`.
- `--single-thread` – run using a single worker thread.
- `--max-threads <n>` – cap the scanning worker threads.
- `--cpu-percent <n>` – approximate CPU usage limit (1–100).
- `--cpu-cores <mask>` – set CPU affinity mask (e.g. `0x3` binds to cores 0 and 1).
- `--cpu-poll <s>` – how often to sample CPU usage in seconds (default 5).
- `--mem-limit <MB>` – abort if memory usage exceeds this amount.
- `--check-only` – only check for updates without pulling.
- `--no-hash-check` – always pull without comparing commit hashes first.
- `--no-cpu-tracker` – disable CPU usage display in the TUI.
- `--no-mem-tracker` – disable memory usage display in the TUI.
- `--no-thread-tracker` – disable thread count display in the TUI.
- `--net-tracker` – show total network usage since startup.
- `--cli` – output text updates to stdout instead of the TUI.
- `--silent` – disable all console output; only logs are written.
- `--help` – show the usage information and exit.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log
is written to a timestamped file inside this directory and its location is shown in the TUI.
Use `--log-file <path>` to append high level messages to the given file. The program records startup, repository actions and shutdown there. For example:
`./autogitpull myprojects --log-dir logs --log-file autogitpull.log --log-level DEBUG`
CPU, memory and thread usage are tracked and shown by default. Disable them individually with `--no-cpu-tracker`, `--no-mem-tracker` or `--no-thread-tracker`. Enable network usage tracking with `--net-tracker`.

## Linting

The project uses `clang-format` and `cpplint` (configured via `CPPLINT.cfg`) to
enforce a consistent code style. Run `make lint` before committing to ensure
formatting and style rules pass:

```bash
make lint
```

The CI workflow also executes this command and will fail on formatting or lint errors.

Markdown and JSON files are formatted with [Prettier](https://prettier.io/) using
the settings in `.prettierrc`. Format them with:

```bash
npm run format
```

`make lint` also checks these files via `prettier --check`.

### Status labels

When the program starts, each repository is listed with the **Pending** status
until it is checked for the first time. Once a scan begins the status switches
to **Checking** and later reflects the pull result.

## Runtime requirements

- **Git** must be available in your `PATH` for libgit2 to interact with repositories.
- Network access is required to contact remote Git servers when pulling updates.
- The application prints ANSI color codes; on Windows run it in a terminal that
  supports color (e.g. Windows Terminal or recent PowerShell).

## Licensing

autogitpull is licensed under the MIT license (see `LICENSE`). The project
bundles the license for the statically linked libgit2 library in
`LIBGIT2_LICENSE`.
