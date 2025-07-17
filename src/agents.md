# Source Overview
This directory contains the main application sources. Each file has a specific role:
- `autogitpull.cpp` – executable entry point and core workflow.
- `config_utils.cpp` – load configuration files.
- `git_utils.cpp` – git operations via libgit2.
- `tui.cpp` – text-based user interface.
- `logger.cpp` – logging utilities.
- `resource_utils.cpp` – CPU, memory and thread tracking.
- `system_utils.cpp` – wrappers for system calls.
- `time_utils.cpp` – timers and helpers.
- `debug_utils.cpp` – debugging helpers.
- `options.cpp` – defines CLI options.
- `parse_utils.cpp` – command-line argument parsing.
