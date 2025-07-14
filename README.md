# autogitpull
Automatic Git Puller & Monitor

## Dependencies
This tool relies on [libgit2](https://libgit2.org/). On Linux install the
`libgit2-dev` package before compiling. Windows builds require `libgit2.lib`
to be available on the library path.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>] [--help]`

Available options:

* `--include-private` – include private or non-GitHub repositories in the scan.
* `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
* `--interval <seconds>` – delay between automatic scans (default 60).
* `--log-dir <path>` – directory where pull logs will be written.
* `--help` – show the usage information and exit.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.
