# autogitpull
Automatic Git Puller & Monitor

## Dependencies
This tool relies on [libgit2](https://libgit2.org/). Run `install_deps.sh` on Linux or macOS, or `install_deps.bat` on Windows to download and install `libgit2` automatically. These scripts are also invoked from the compile scripts when the library is missing.

## Building
Run `make` to compile the project using g++ with C++17 and pthread support.
The resulting executable is named `autogitpull`.

Use `make clean` to remove the compiled binary and object files.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>] [--help]`

Available options:

* `--include-private` – include private or non-GitHub repositories in the scan.
* `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
* `--interval <seconds>` – delay between automatic scans (default 60).
* `--log-dir <path>` – directory where pull logs will be written.
* `--help` – show the usage information and exit.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log
is written to a timestamped file inside this directory and its location is shown in the TUI.
