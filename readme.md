<img width="128" height="128" alt="icon_128" style="text-align:center" src="https://github.com/user-attachments/assets/6ed0496b-665d-403f-a50c-9c3fe725facd" />

# autogitpull

Automatic Git Puller & Monitor

Tested and working on MacOS, Ubuntu and Windows

`autogitpull` scans a directory of Git repositories, pulls updates on a schedule
and shows progress either in an interactive TUI or with plain console output.

<img width="625" height="646" alt="image" src="https://github.com/user-attachments/assets/678db386-a9b8-4e39-9ae5-ef1ea762ae7d" />

## Features

- Periodic scanning and automatic `git pull`
 - Optional CLI mode that prints logs without the TUI
- Very lightweight, low resource usage
- YAML or JSON configuration files
- Detailed logging with resource tracking
- Throttling and CPU/memory limits
- Automatically resumes after system sleep/hibernation

## Usage

`autogitpull <root-folder> [--include-private] [--show-skipped] [--show-notgit] [--show-version] [--version] [--interval <seconds>] [--refresh-rate <ms|s|m>] [--cpu-poll <N[s|m|h|d|w|M]>] [--mem-poll <N[s|m|h|d|w|M]>] [--thread-poll <N[s|m|h|d|w|M]>] [--log-dir <path>] [--log-file <path>] [--max-log-size <bytes>] [--include-dir <dir>] [--ignore <dir>] [--recursive] [--max-depth <n>] [--log-level <level>] [--verbose] [--concurrency <n>] [--threads <n>] [--single-thread] [--max-threads <n>] [--cpu-percent <n.n>] [--cpu-cores <mask>] [--mem-limit <M/G>] [--check-only] [--no-hash-check] [--no-cpu-tracker] [--no-mem-tracker] [--no-thread-tracker] [--net-tracker] [--download-limit <KB/MB>] [--upload-limit <KB/MB>] [--disk-limit <KB/MB>] [--total-traffic-limit <KB/MB/GB>] [--cli] [--single-run] [--silent] [--force-pull] [--remove-lock] [--hard-reset] [--confirm-reset] [--confirm-alert] [--sudo-su] [--debug-memory] [--dump-state] [--dump-large <n>] [--attach <name>] [--background <name>] [--reattach <name>] [--persist[=name]] [--help]`

### TLDR usage tips

- For minimum memory footprint use `--single-thread`, trade off on performance/speed.
- To override/discard local changes every time use `--force-pull`, it's basically to sync with remote.
- To only sync the latest repos you're working on use `--updated-since` 6h, to only sync repos updated in the last 6 hours.
- To only show dates from repos that have been synced during the current session use `--session-dates-only`.

Repositories with uncommitted changes are skipped by default to avoid losing work. Use `--force-pull` (alias: `--discard-dirty`) to reset such repositories to the remote state.

### Usage options

Most options have single-letter shorthands. Run `autogitpull --help` for a complete list.
The full catalogue of flags with their default values is documented in
[`docs/cli_options.md`](docs/cli_options.md).

#### Basics
- `--include-private` (`-p`) – Include private repositories.
- `--root` (`-o`) `<path>` – Root folder of repositories.
- `--interval` (`-i`) `<sec>` – Delay between scans.
- `--refresh-rate` (`-r`) `<ms|s|m>` – TUI refresh rate.
- `--recursive` (`-e`) – Scan subdirectories recursively.
- `--max-depth` (`-D`) `<n>` – Limit recursive scan depth.
- `--include-dir` `<dir>` – Additional directory to scan (repeatable).
- `--ignore` (`-I`) `<dir>` – Directory to ignore (repeatable).
- `--single-run` (`-u`) – Run a single scan cycle and exit.
- `--single-repo` (`-S`) – Only monitor the specified root repo.
- `--rescan-new` (`-w`) `<min>` – Rescan for new repos periodically.
- `--wait-empty` (`-W`) `[n]` – Keep retrying when no repos are found, up to an optional limit.
- `--dont-skip-timeouts` – Retry repositories that timeout.
- `--keep-first-valid` – Keep valid repos from the first scan even if missing.
- `--updated-since` `<N[m|h|d|w|M]>` – Only sync repos updated recently.
- `--help` (`-h`) – Show this message.

#### Display
- `--show-skipped` (`-k`) – Show skipped repositories.
- `--show-notgit` – Show non-git directories.
- `--show-version` (`-v`) – Display program version in TUI.
- `--version` (`-V`) – Print program version and exit.
- `--show-runtime` – Display elapsed runtime.
- `--show-repo-count` (`-Z`) – Display number of repositories.
- `--show-commit-date` (`-T`) – Display last commit time.
- `--show-commit-author` (`-U`) – Display last commit author.
- `--show-pull-author` – Show author when pull succeeds.
- `--session-dates-only` – Only show dates for repos pulled this session.
- `--hide-date-time` – Hide date/time line in TUI.
- `--hide-header` (`-H`) – Hide status header.
- `--row-order` `<mode>` – Row ordering (alpha/reverse).
- `--color` `<ansi>` – Override status color.
- `--no-colors` (`-C`) – Disable ANSI colors.
- `--censor-names` – Mask repository names in output.
- `--censor-char` `<ch>` – Character used for masking.

#### Config
- `--auto-config` – Auto detect YAML or JSON config.
- `--auto-reload-config` – Reload config when the file changes.
- `--rerun-last` – Reuse args from `.autogitpull.config`.
- `--save-args` – Save args to config file.
- `--enable-history[=<file>]` – Enable command history (default `.autogitpull.config`).
- `--config-yaml` (`-y`) `<file>` – Load options from YAML file.
- `--config-json` (`-j`) `<file>` – Load options from JSON file.

#### Process
- `--cli` (`-c`) – Use console output.
- `--silent` (`-s`) – Disable console output.
- `--attach` (`-A`) `<name>` – Attach to daemon and show status.
- `--background` (`-b`) `<name>` – Run in background with attach name.
- `--reattach` (`-B`) `<name>` – Reattach to background process.
- `--persist` (`-P`) `[name]` – Keep running after exit; optional run name.
- `--respawn-limit` `<n[,min]>` – Respawn limit within minutes.
- `--max-runtime` `<sec>` – Exit after given runtime.
- `--pull-timeout` (`-O`) `<sec>` – Network operation timeout.
- `--exit-on-timeout` – Terminate worker if a poll exceeds the timeout.
- `--print-skipped` – Print skipped repositories once.
- `--keep-first` – Keep repositories validated on the first scan.

#### Logging
- `--log-dir` (`-d`) `<path>` – Directory for pull logs.
- `--log-file` (`-l`) `<path>` – File for general logs.
- `--max-log-size` `<bytes>` – Rotate `--log-file` when over this size.
- `--log-level` (`-L`) `<level>` – Set log verbosity.
- `--verbose` (`-g`) – Shortcut for DEBUG logging.
- `--debug-memory` (`-m`) – Log memory usage each scan.
- `--dump-state` – Dump container state when large.
- `--dump-large` `<n>` – Dump threshold for `--dump-state`.
- `--syslog` – Log to syslog.
- `--syslog-facility` `<n>` – Syslog facility.

#### Concurrency
- `--concurrency` (`-n`) `<n>` – Number of worker threads.
- `--threads` (`-t`) `<n>` – Alias for `--concurrency`.
- `--single-thread` (`-q`) – Run using a single worker thread.
- `--max-threads` (`-M`) `<n>` – Cap the scanning worker threads.

#### Resource limits
- `--cpu-percent` (`-E`) `<n.n>` – Approximate CPU usage limit.
- `--cpu-cores` `<mask>` – Set CPU affinity mask.
- `--mem-limit` (`-Y`) `<M/G>` – Abort if memory exceeds this amount.
- `--download-limit` `<KB/MB>` – Limit total download rate.
- `--upload-limit` `<KB/MB>` – Limit total upload rate.
- `--disk-limit` `<KB/MB>` – Limit disk throughput.
- `--total-traffic-limit` `<KB/MB/GB>` – Stop after transferring this much data.

#### Tracking
- `--cpu-poll` `<N[s|m|h|d|w|M]>` – CPU polling interval.
- `--mem-poll` `<N[s|m|h|d|w|M]>` – Memory polling interval.
- `--thread-poll` `<N[s|m|h|d|w|M]>` – Thread count interval.
- `--no-cpu-tracker` (`-X`) – Disable CPU usage tracker.
- `--no-mem-tracker` – Disable memory usage tracker.
- `--no-thread-tracker` – Disable thread tracker.
- `--net-tracker` – Track network usage.
- `--vmem` – Show virtual memory usage.

#### Actions
- `--check-only` (`-x`) – Only check for updates.
- `--no-hash-check` (`-N`) – Always pull without hash check.
- `--force-pull` (`-f`) – Discard local changes when pulling.
- `--discard-dirty` – Alias for `--force-pull`.
- `--install-daemon` – Install background daemon.
- `--uninstall-daemon` – Uninstall background daemon.
- `--daemon-config` `<file>` – Config file for daemon install.
- `--install-service` – Install system service.
- `--uninstall-service` – Uninstall system service.
- `--start-service` – Start installed service.
- `--stop-service` – Stop installed service.
- `--force-stop-service` – Force stop service.
- `--restart-service` – Restart service.
- `--force-restart-service` – Force restart service.
- `--service-status` – Check if the service exists and is running.
- `--start-daemon` – Start daemon unit.
- `--stop-daemon` – Stop daemon unit.
- `--force-stop-daemon` – Force stop daemon.
- `--restart-daemon` – Restart daemon unit.
- `--force-restart-daemon` – Force restart daemon.
- `--daemon-status` – Check if the daemon exists and is running.
- `--service-config` `<file>` – Config file for service install.
- `--remove-lock` (`-R`) – Remove directory lock file and exit.
- `--kill-all` – Terminate running instance and exit.
- `--list-services` – List installed service units.
- `--list-daemons` – Alias for `--list-services`.
- `--ignore-lock` – Don't create or check lock file.
- `--hard-reset` – Delete all logs and configs.
- `--confirm-reset` – Confirm `--hard-reset`.
- `--confirm-alert` – Confirm unsafe interval or force pull.
- `--sudo-su` – Suppress confirmation alerts.


By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log is written to a timestamped file inside this directory and its location is shown in the TUI. Use `--log-file <path>` to append high level messages to the given file.

### Persistent mode

Run with `--persist` to automatically restart `autogitpull` whenever the main
worker exits. This keeps the application alive if it is terminated while the
computer is under heavy load or resumes from sleep or hibernation. Unhandled
errors from the worker are caught and logged so the monitor can restart the
process cleanly. Optionally specify a name like `--persist=myrun` to tag the
instance. The `--respawn-limit` option controls how many restarts are allowed
within a time window (set it to `0` for no limit).

### YAML configuration

Frequently used options can be stored in a YAML file and loaded with `--config-yaml <file>`.
Keys match the long option names without the leading dashes. Boolean flags should be set to `true` or `false`.
Arguments provided on the command line override values from the YAML file. See `examples/example-config.yaml` and `examples/example-config.json` for complete examples.

### JSON configuration

Settings can also be provided in JSON format and loaded with `--config-json <file>`.
The keys mirror the long command line options without the leading dashes. Values from the command line override those from the JSON file. See `examples/example-config.json` for a complete example.

### Repository overrides

Sections whose keys are repository paths provide per-repository overrides. Any option supported on the command line may be placed under a path key to override the global value for that repository.

```yaml
/home/user/repos/foo:
  force-pull: true
  download-limit: 100
```

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

Run `scripts/install_libgit2_mingw.bat` to build libgit2 and yaml-cpp natively
with MinGW. The script installs the static libraries and headers under the
`libs` directory and also downloads the header-only `nlohmann-json` library to
complete the dependencies.

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
For an extra-small Windows binary run `scripts/compile-compress.bat` which
reuses `compile.bat` with size optimizations and then compresses the result
with UPX.

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

macOS builds automatically fall back to the header-only implementation
`include/thread_compat.hpp` when the system libc++ lacks `std::jthread`.

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
`make test` requires the development packages for `libgit2`, `yaml-cpp`,
`nlohmann-json` and `zlib`. Use `scripts/install_deps.sh` (Linux/macOS) or
`scripts/install_deps.bat` (Windows) to install them before configuring and
building the tests.
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

### Icon generation

Run `scripts/generate_icons.sh` (Linux/macOS) or `scripts/generate_icons.bat`
(Windows) to create platform icons from `graphics/icon.png`. If ImageMagick's
`magick` command is missing, the Unix script attempts to install the
`imagemagick` package using `apt`, `dnf`, or `yum`. Windows relies on
`winget` to install ImageMagick automatically. Windows builds embed the
generated `icon.ico` and macOS uses `icon.icns`.

## Linting

The project uses `clang-format` and `cpplint` (configured via `CPPLINT.cfg`) to
enforce a consistent code style. Run `make lint` before committing to ensure
formatting and style rules pass:

```bash
make lint
```

The CI workflow also executes this command and will fail on formatting or lint errors.

`scripts/install_deps.sh` and `scripts/install_deps.bat` automatically install
`cpplint` if it is missing.

### Status labels

When the program starts, each repository is listed with the **Pending** status
until it is checked for the first time. Once a scan begins the status switches
to **Checking** and later reflects the pull result.

### Versioning

autogitpull uses a rolling release model. Each push to the `main` branch
triggers the CI workflow that tags the commit with a date-based string such as
`2025.07.31-1`. The `--version` flag prints this tag so the program is always
identified by the latest CI release.

## Production requirements

- **Git** must be available in your `PATH` for libgit2 to interact with repositories.
- Network access is required to contact remote Git servers when pulling updates.
- The application prints ANSI color codes; on Windows run it in a terminal that
  supports color (e.g. Windows Terminal or recent PowerShell).

## Licensing

autogitpull is licensed under the MIT license (see `LICENSE`). The project
bundles the license texts for its third-party dependencies under
the `licenses/` directory, including `libgit2.txt`, `yaml-cpp.txt`,
`nlohmann-json.txt` and `zlib.txt`.

