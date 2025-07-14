# autogitpull
Automatic Git Puller & Monitor

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped]`

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Pass `--include-private` after the root folder to include these repositories. Skipped repositories are hidden from the TUI unless you also pass `--show-skipped`.
