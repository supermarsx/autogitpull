# autogitpull
Automatic Git Puller & Monitor

## Dependencies
This tool relies on [libgit2](https://libgit2.org/). On Linux install the
`libgit2-dev` package before compiling. Windows builds require `libgit2.lib`
to be available on the library path.

## Building
Run `make` to compile the project using g++ with C++17 and pthread support.
The resulting executable is named `autogitpull`.

Use `make clean` to remove the compiled binary and object files.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>]`

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Pass `--include-private` after the root folder to include these repositories. Skipped repositories are hidden from the TUI unless you also pass `--show-skipped`.

Use `--interval` to specify the delay in seconds between automatic scans (default is 60 seconds).

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log
is written to a timestamped file inside this directory and its location is shown in the TUI.
