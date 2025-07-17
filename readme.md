# autogitpull

Automatic Git Puller & Monitor

`autogitpull` scans a directory of Git repositories, pulls updates on a schedule
and shows progress either in an interactive TUI or with plain console output.

## Features

- Periodic scanning and automatic `git pull`
- Optional CLI mode without the TUI
- YAML or JSON configuration files
- Detailed logging with resource tracking
- Throttling and CPU/memory limits

## Usage

`autogitpull <root-folder> [--include-private] [--show-skipped] [--show-version] [--version] [--interval <seconds>] [--refresh-rate <ms>] [--cpu-poll <s>] [--mem-poll <s>] [--thread-poll <s>] [--log-dir <path>] [--log-file <path>] [--ignore <dir>] [--recursive] [--max-depth <n>] [--log-level <level>] [--verbose] [--concurrency <n>] [--threads <n>] [--single-thread] [--max-threads <n>] [--cpu-percent <n>] [--cpu-cores <mask>] [--mem-limit <MB>] [--check-only] [--no-hash-check] [--no-cpu-tracker] [--no-mem-tracker] [--no-thread-tracker] [--net-tracker] [--download-limit <KB/s>] [--upload-limit <KB/s>] [--disk-limit <KB/s>] [--cli] [--single-run] [--silent] [--force-pull] [--debug-memory] [--dump-state] [--dump-large <n>] [--help]`

Most options have single-letter shorthands. Run `autogitpull --help` to see a concise list.

Available options:

- `--include-private` – include private or non-GitHub repositories in the scan.
- `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
- `--show-version` – display the program version in the TUI header.
- `--version` – print the program version and exit.
- `--interval <seconds>` – delay between automatic scans (default 30).
- `--refresh-rate <ms>` – how often the TUI refreshes in milliseconds (default 250).
- `--log-dir <path>` – directory where pull logs will be written.
- `--log-file <path>` – file for general messages.
- `--ignore <dir>` – skip the given directory when collecting repositories. This option may be repeated.
- `--recursive` – search subdirectories recursively for repositories.
- `--max-depth <n>` – limit recursion depth; implies `--recursive`.
- `--log-level <level>` – minimum message level written to the log (`DEBUG`, `INFO`, `WARNING`, `ERROR`).
- `--verbose` – shorthand for `--log-level DEBUG`.
- `--concurrency <n>` – number of repositories processed in parallel (default: hardware concurrency).
- `--threads <n>` – alias for `--concurrency`.
- `--single-thread` – run using a single worker thread.
- `--max-threads <n>` – cap the scanning worker threads.
- `--cpu-percent <n>` – approximate CPU usage limit (1–100).
- `--cpu-cores <mask>` – set CPU affinity mask (e.g. `0x3` binds to cores 0 and 1).
- `--cpu-poll <s>` – how often to sample CPU usage in seconds (default 5).
- `--mem-poll <s>` – how often to sample memory usage in seconds (default 5).
- `--thread-poll <s>` – how often to sample thread count in seconds (default 5).
- `--mem-limit <MB>` – abort if memory usage exceeds this amount.
- `--check-only` – only check for updates without pulling.
- `--no-hash-check` – always pull without comparing commit hashes first.
- `--no-cpu-tracker` – disable CPU usage display in the TUI.
- `--no-mem-tracker` – disable memory usage display in the TUI.
- `--no-thread-tracker` – disable thread count display in the TUI.
- `--net-tracker` – show total network usage since startup.
- `--disk-limit <KB/s>` – throttle disk reads and writes.
- `--cli` – output text updates to stdout instead of the TUI.
- `--single-run` – perform one scan cycle and exit.
- `--silent` – disable all console output; only logs are written.
- `--force-pull` – discard local changes when pulling updates.
- `--debug-memory` – log container sizes and memory usage after each scan.
- `--dump-state` – dump repository state when container size exceeds a limit.
- `--dump-large <n>` – threshold for dumping containers with `--dump-state`.
- `--help` – show the usage information and exit.

Repositories with uncommitted changes are skipped by default to avoid losing work. Use `--force-pull` to reset such repositories to the remote state.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log is written to a timestamped file inside this directory and its location is shown in the TUI. Use `--log-file <path>` to append high level messages to the given file.

### YAML configuration

Frequently used options can be stored in a YAML file and loaded with `--config-yaml <file>`.
Keys match the long option names without the leading dashes. Boolean flags should be set to `true` or `false`.
Arguments provided on the command line override values from the YAML file. See `examples/example-config.yaml` and `examples/example-config.json` for complete examples.

### JSON configuration

Settings can also be provided in JSON format and loaded with `--config-json <file>`.
The keys mirror the long command line options without the leading dashes. Values from the command line override those from the JSON file. See `examples/example-config.json` for a complete example.

## Build requirements

This tool relies on [libgit2](https://libgit2.org/). The helper scripts
`scripts/install_deps.sh` (Linux/macOS) and `scripts/install_deps.bat` (Windows) automatically
download and install `libgit2` when needed. You can also run `make deps` on
Unix-like systems to invoke the installer. The build requires the development
package (named `libgit2-dev` on Debian/Ubuntu). The Makefile tries to link
statically but falls back to dynamic linking when static libraries are
unavailable. If you prefer to install the library yourself, follow the
instructions below.

Running the unit tests (`make test`) additionally requires the development
headers and libraries for `yaml-cpp` and
[nlohmann/json](https://github.com/nlohmann/json). The same installer scripts
(`scripts/install_deps.sh` or `scripts/install_deps.bat`) will install these packages along with
`libgit2`.

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

Run `scripts/install_libgit2_mingw.bat` to build libgit2 natively with MinGW. The
script installs the static library and headers under `libs/libgit2_install`.

`scripts/compile-cl.bat` expects a vcpkg installation while `scripts/compile.bat` uses the
library produced by `scripts/install_libgit2_mingw.bat` and will call it
automatically if `libs/libgit2_install` is missing. When linking with MinGW,
additional Windows system libraries are required. `scripts/compile.bat` now attempts
to install MinGW through Chocolatey if `g++` is not found and already
includes `winhttp`, `ole32`, `rpcrt4` and `crypt32` so that the build
succeeds without manual tweaks.

## Building

### Using the provided scripts

Run `make` (Linux/macOS), `scripts/compile.bat` (MinGW) or `scripts/compile-cl.bat` (MSVC) to
build the project. `scripts/compile.bat` invokes `scripts/install_libgit2_mingw.bat` when
`libgit2` is missing. All helper scripts place the resulting executable in the `dist/`
directory as `dist/autogitpull` (or `dist/autogitpull.exe` on Windows).

The repository also ships with `scripts/compile.sh` for Unix-like environments which
will attempt to install a C++ compiler if one isn't present. Windows users get
`scripts/compile.bat` (MinGW) and `scripts/compile-cl.bat` (MSVC) along with
`scripts/install_deps.bat`, `scripts/install_libgit2_mingw.bat` and `scripts/run.bat`.

Clean up intermediate files with `make clean`. Dedicated cleanup scripts are also
available under `scripts/` as `scripts/clean.sh` (Unix-like systems) and `scripts/clean.bat`
(Windows) to remove the generated binary, object files and the `build` and `dist` directories.

### Debug builds for leak analysis

The scripts `scripts/compile-debug.sh`, `scripts/compile-debug.bat` and `scripts/compile-debug-cl.bat`
compile the program with AddressSanitizer and debug information enabled. They
produce `dist/autogitpull_debug` (or `dist/autogitpull_debug.exe` on Windows). Use these
builds when running leak detection tools:

```bash
scripts/compile-debug.sh      # Linux/macOS
scripts/compile-debug.bat       # MinGW
scripts/compile-debug-cl.bat    # MSVC
```

### Manual compilation

If you prefer to build without the helper scripts, the following commands show
the bare minimum required to compile the program.

On Linux with `g++`:

```bash
g++ -std=c++20 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp \
    $(pkg-config --cflags libgit2) \
    $(pkg-config --static --libs libgit2 2>/dev/null || pkg-config --libs libgit2) \
    -pthread -o dist/autogitpull
```

On macOS with `clang++`:

```bash
clang++ -std=c++20 autogitpull.cpp git_utils.cpp tui.cpp logger.cpp $(pkg-config --cflags --libs libgit2) -pthread -o dist/autogitpull
```

On Windows with MSVC's `cl`:

```batch
cl /std:c++20 /EHsc /MT /Ipath\to\libgit2\include autogitpull.cpp git_utils.cpp tui.cpp logger.cpp ^
    /link /LIBPATH:path\to\libgit2\lib git2.lib /Fedist\autogitpull.exe
```

These commands mirror what the scripts do internally.

### Building with CMake

Alternatively, configure the project with CMake:

```bash
cmake -S . -B build
cmake --build build
```

The resulting executable will appear in the `dist/` directory.

### Running tests

Unit tests use [Catch2](https://github.com/catchorg/Catch2). If the library is
not installed, CMake will automatically download it using `FetchContent`.
`make test` requires the development packages for `libgit2`, `yaml-cpp` and
`nlohmann-json`. Use `scripts/install_deps.sh` (Linux/macOS) or `scripts/install_deps.bat`
(Windows) to install them before configuring and building the tests.
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



## Linting

The project uses `clang-format` and `cpplint` (configured via `CPPLINT.cfg`) to
enforce a consistent code style. Run `make lint` before committing to ensure
formatting and style rules pass:

```bash
make lint
```

The CI workflow also executes this command and will fail on formatting or lint errors.

### Status labels

When the program starts, each repository is listed with the **Pending** status
until it is checked for the first time. Once a scan begins the status switches
to **Checking** and later reflects the pull result.

## Production requirements

- **Git** must be available in your `PATH` for libgit2 to interact with repositories.
- Network access is required to contact remote Git servers when pulling updates.
- The application prints ANSI color codes; on Windows run it in a terminal that
  supports color (e.g. Windows Terminal or recent PowerShell).

## Licensing

autogitpull is licensed under the MIT license (see `LICENSE`). The project
bundles the license texts for its third-party dependencies under
the `licenses/` directory, including `libgit2.txt`, `yaml-cpp.txt` and
`nlohmann-json.txt`.
