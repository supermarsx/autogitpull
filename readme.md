<img width="128" height="128" alt="icon_128" style="text-align:center" src="https://github.com/user-attachments/assets/6ed0496b-665d-403f-a50c-9c3fe725facd" />

# autogitpull

[![Downloads](https://img.shields.io/github/downloads/supermarsx/autogitpull/total)](https://github.com/supermarsx/autogitpull/releases)
[![Download Latest](https://img.shields.io/github/downloads/supermarsx/autogitpull/latest/total)](https://github.com/supermarsx/autogitpull/releases/latest)
[![Issues](https://img.shields.io/github/issues/supermarsx/autogitpull)](https://github.com/supermarsx/autogitpull/issues)
[![Stars](https://img.shields.io/github/stars/supermarsx/autogitpull?style=social)](https://github.com/supermarsx/autogitpull/stargazers)
[![Forks](https://img.shields.io/github/forks/supermarsx/autogitpull?style=social)](https://github.com/supermarsx/autogitpull/network/members)
[![Watchers](https://img.shields.io/github/watchers/supermarsx/autogitpull?style=social)](https://github.com/supermarsx/autogitpull/watchers)
[![Commit activity](https://img.shields.io/github/commit-activity/m/supermarsx/autogitpull)](https://github.com/supermarsx/autogitpull/graphs/commit-activity)
[![Commit total](https://img.shields.io/github/commit-activity/t/supermarsx/autogitpull)](https://github.com/supermarsx/autogitpull/graphs/commit-activity)
[![Coverage](https://raw.githubusercontent.com/supermarsx/autogitpull/badges/coverage.svg)](https://github.com/supermarsx/autogitpull/actions/workflows/coverage.yml)
[![Made with C++](https://img.shields.io/badge/Made%20with-C%2B%2B-1f425f.svg)](https://isocpp.org/)
[![License](https://img.shields.io/github/license/supermarsx/autogitpull)](license.md)

CI Status

[![Format Check](https://github.com/supermarsx/autogitpull/actions/workflows/format.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/format.yml)
[![Auto Format](https://github.com/supermarsx/autogitpull/actions/workflows/auto-format.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/auto-format.yml)
[![Lint](https://github.com/supermarsx/autogitpull/actions/workflows/lint.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/lint.yml)
[![Tests](https://github.com/supermarsx/autogitpull/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/tests.yml)
[![Build](https://github.com/supermarsx/autogitpull/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/build.yml)


[![Rolling Release](https://github.com/supermarsx/autogitpull/actions/workflows/rolling-release.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/rolling-release.yml)
[![Release](https://github.com/supermarsx/autogitpull/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/supermarsx/autogitpull/actions/workflows/release.yml)

Quick Downloads

Latest Release

[![Windows x64](https://img.shields.io/badge/Download-Windows%20x64-0078D6?logo=windows&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-windows-x64.exe)
[![Windows ARM64](https://img.shields.io/badge/Download-Windows%20ARM64-0078D6?logo=windows&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-windows-arm64.exe)
[![macOS x64](https://img.shields.io/badge/Download-macOS%20x64-000000?logo=apple&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-macos-x64)
[![macOS ARM64](https://img.shields.io/badge/Download-macOS%20ARM64-000000?logo=apple&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-macos-arm64)
[![Linux x64](https://img.shields.io/badge/Download-Linux%20x64-2ea44f?logo=linux&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-ubuntu-x64)
[![Linux ARM64](https://img.shields.io/badge/Download-Linux%20ARM64-2ea44f?logo=linux&logoColor=white)](https://github.com/supermarsx/autogitpull/releases/latest/download/autogitpull-ubuntu-arm64)

Rolling Release (pre-release)

[![Windows x64 (Rolling)](https://img.shields.io/badge/Rolling-Windows%20x64-FFA500?logo=windows&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)
[![Windows ARM64 (Rolling)](https://img.shields.io/badge/Rolling-Windows%20ARM64-FFA500?logo=windows&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)
[![macOS x64 (Rolling)](https://img.shields.io/badge/Rolling-macOS%20x64-FFA500?logo=apple&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)
[![macOS ARM64 (Rolling)](https://img.shields.io/badge/Rolling-macOS%20ARM64-FFA500?logo=apple&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)
[![Linux x64 (Rolling)](https://img.shields.io/badge/Rolling-Linux%20x64-FFA500?logo=linux&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)
[![Linux ARM64 (Rolling)](https://img.shields.io/badge/Rolling-Linux%20ARM64-FFA500?logo=linux&logoColor=white)](https://github.com/supermarsx/autogitpull/releases?q=rolling-v&expanded=true)


> The coverage badge reflects the latest report generated by CI. Full coverage summaries (HTML, XML, and text) are published as
> a `coverage-report` artifact on each run, and the badge graphic is synced to the `badges` branch.

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

`autogitpull <root-folder> [--include-private] [--show-skipped] [--show-notgit] [--show-version] [--version] [--interval <N[s|m|h|d|w|M|Y]>] [--refresh-rate <ms|s|m>] [--cpu-poll <N[s|m|h|d|w|M|Y]>] [--mem-poll <N[s|m|h|d|w|M|Y]>] [--thread-poll <N[s|m|h|d|w|M|Y]>] [--log-dir <path>] [--log-file <path>] [--max-log-size <bytes>] [--include-dir <dir>] [--ignore <dir>] [--recursive] [--max-depth <n>] [--log-level <level>] [--verbose] [--concurrency <n>] [--threads <n>] [--single-thread] [--max-threads <n>] [--cpu-percent <n.n>] [--cpu-cores <mask>] [--mem-limit <M/G>] [--check-only] [--no-hash-check] [--no-cpu-tracker] [--no-mem-tracker] [--no-thread-tracker] [--net-tracker] [--download-limit <KB/MB>] [--upload-limit <KB/MB>] [--disk-limit <KB/MB>] [--total-traffic-limit <KB/MB/GB>] [--cli] [--single-run] [--silent] [--force-pull] [--remove-lock] [--hard-reset] [--confirm-reset] [--confirm-alert] [--sudo-su] [--debug-memory] [--dump-state] [--dump-large <n>] [--attach <name>] [--background <name>] [--reattach <name>] [--persist[=name]] [--help]`

### TLDR usage tips

- For minimum memory footprint use `--single-thread`, trade off on performance/speed.
- To override and discard local changes every time, use `--force-pull`; uncommitted work is erased as repositories reset to remote.
- To only sync the latest repos you're working on use `--updated-since` 6h, to only sync repos updated in the last 6 hours.
- To only show dates from repos that have been synced during the current session use `--session-dates-only`.

Repositories with uncommitted changes are skipped by default to avoid losing work. Use `--force-pull` (alias: `--discard-dirty`) to reset such repositories to the remote state, permanently deleting their uncommitted changes.

### Usage options

Most options have single-letter shorthands. Run `autogitpull --help` for a complete list.
The full catalogue of flags with their default values is documented in
[`docs/cli_options.md`](docs/cli_options.md).

#### Basics
- `--include-private` (`-p`) – Include private repositories.
- `--root` (`-o`) `<path>` – Root folder of repositories.
- `--interval` (`-i`) `<N[s|m|h|d|w|M|Y]>` – Delay between scans.
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
- `--updated-since` `<N[m|h|d|w|M|Y]>` – Only sync repos updated recently.
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
- `--auto-reload-config` – Reload config when the file changes (disabled by default).
- `--rerun-last` – Reuse args from `.autogitpull.config`.
- `--save-args` – Save args to config file.
- `--enable-history[=<file>]` – Enable command history (default `.autogitpull.config`).
- `--config-yaml` (`-y`) `<file>` – Load options from YAML file.
- `--config-json` (`-j`) `<file>` – Load options from JSON file.

#### Authentication
- `--credential-file` `<file>` – Read username and password from file.
- `--ssh-public-key` `<file>` – Path to SSH public key.
- `--ssh-private-key` `<file>` – Path to SSH private key.

#### Process
- `--cli` (`-c`) – Use console output.
- `--silent` (`-s`) – Disable console output.
- `--attach` (`-A`) `<name>` – Attach to daemon and show status.
- `--background` (`-b`) `<name>` – Run in background with attach name.
- `--reattach` (`-B`) `<name>` – Reattach to background process.
- `--persist` (`-P`) `[name]` – Keep running after exit; optional run name.
- `--respawn-limit` `<n[,min]>` – Respawn limit within minutes.
- `--max-runtime` `<N[s|m|h|d|w|M|Y]>` – Exit after given runtime.
- `--pull-timeout` (`-O`) `<N[s|m|h|d|w|M|Y]>` – Network operation timeout.
- `--exit-on-timeout` – Terminate worker if a poll exceeds the timeout.
- `--print-skipped` – Print skipped repositories once.
- `--keep-first` – Keep repositories validated on the first scan.

#### Daemon management
On macOS and Linux, `autogitpull` can run as a background service via
`launchd` or `systemd`:

- `--install-daemon` – install the service unit.
- `--uninstall-daemon` – remove the service unit.
- `--start-daemon` / `--stop-daemon` – control the service.
- `--daemon-status` – check whether it is installed and running.

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
- `--cpu-poll` `<N[s|m|h|d|w|M|Y]>` – CPU polling interval.
- `--mem-poll` `<N[s|m|h|d|w|M|Y]>` – Memory polling interval.
- `--thread-poll` `<N[s|m|h|d|w|M|Y]>` – Thread count interval.
- `--no-cpu-tracker` (`-X`) – Disable CPU usage tracker.
- `--no-mem-tracker` – Disable memory usage tracker.
- `--no-thread-tracker` – Disable thread tracker.
- `--net-tracker` – Track network usage.
- `--vmem` – Show virtual memory usage.

#### Actions
- `--check-only` (`-x`) – Only check for updates.
- `--no-hash-check` (`-N`) – Always pull without hash check.
- `--force-pull` (`-f`) – Reset repos to remote state, losing uncommitted changes and untracked files.
- `--discard-dirty` – Alias for `--force-pull`; same data loss.
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
- `--hard-reset` – Remove autogitpull logs, configs, and lock files (cannot be undone).
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

### Per-repository settings

Configuration files may include a `repositories` section that maps repository paths to option overrides. Keys inside each repository entry correspond to long command line options without the leading dashes. The old format that places repository paths at the top level is still supported.

YAML example:

```yaml
root: /home/user/repos
repositories:
  /home/user/repos/foo:
    force-pull: true
    download-limit: 100
```

JSON example:

```json
{
  "root": "/home/user/repos",
  "repositories": {
    "/home/user/repos/foo": {
      "force-pull": true,
      "download-limit": 100
    }
  }
}
```

## Build requirements

This tool relies on [libgit2](https://libgit2.org/) together with
`yaml-cpp`, [nlohmann/json](https://github.com/nlohmann/json), `zlib` and
[Catch2](https://github.com/catchorg/Catch2) for testing. CMake fetches and
builds these third-party libraries automatically through `FetchContent`, so a
supported compiler plus CMake ≥ 3.20 and Git are the only prerequisites for a
standard build. The legacy helper scripts `scripts/install_deps.sh`
(Linux/macOS) and `scripts/install_deps.bat` (Windows) now simply verify that
those tools are available on your `PATH`.

If you prefer to provide system packages manually, follow the optional
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

### One-liner cross-platform build

Prefer CMake directly, but for convenience single-file wrappers are provided:

```bash
# Python wrapper (Linux/macOS/Windows)
python3 scripts/build.py                 # Release build into ./build
python3 scripts/build.py --config Debug  # Debug build
python3 scripts/build.py --test          # Build + run tests (ctest)

# Shell wrapper (Linux/macOS) — delegates to Python if present
bash scripts/build.sh --config RelWithDebInfo -j 8

# PowerShell wrapper (Windows)
pwsh -File scripts/build.ps1 -Config Release -Test
```

All wrappers only invoke CMake/CTest (no custom build logic). On Windows,
run from Developer PowerShell (or any shell with CMake in PATH).

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
The dependency installers remain for backwards compatibility but only perform
basic tool checks now that CMake manages third-party libraries internally.

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

Alternatively, configure the project with the bundled presets:

```bash
cmake --preset release -DCMAKE_EXE_LINKER_FLAGS_RELEASE=-s
cmake --build --preset release -j
# optional install
cmake --install build/release/$(uname)-$(uname -m) --prefix /usr/local
```

The resulting executable will appear in the preset's binary directory
(`build/release/<platform>`). Use `cmake --preset rolling` and `cmake --build
--preset rolling` to create a `RelWithDebInfo` build that matches the
rolling-release pipeline.

### Running tests

Unit tests use [Catch2](https://github.com/catchorg/Catch2) together with the
core dependencies handled by CMake's `FetchContent` flow, so no manual
installation is required for standard builds. After configuring the project,
run `ctest`:

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

Install `cpplint` via your preferred package manager (e.g. `pipx install
cpplint` or `brew install cpplint`) if it is not already available.

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

