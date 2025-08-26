# Command Line Options


## Configuration file schema

Configuration files may be written in YAML or JSON. The top level accepts the
following keys:

| Key | Type | Description |
|-----|------|-------------|
| `interval` | number or string | Maps to `--interval` |
| `cli` | boolean | Maps to `--cli` |
| `root` | string | Maps to `--root` |
| `credential-file` | string | Maps to `--credential-file` |
| `proxy` | string | Maps to `--proxy` |
| `repositories` | object | Map of repository paths to option maps |

All other top-level mappings are treated as option categories or repository
overrides. Scalar keys outside this list are rejected.

## Actions

| Option | Default | Description |
|--------|---------|-------------|
| `--check-only` | false (disabled) | Only check for updates |
| `--dry-run` | false (disabled) | Simulate pull operations without network access |
| `--confirm-alert` | false (disabled) | Confirm unsafe options |
| `--confirm-reset` | false (disabled) | Confirm --hard-reset |
| `--discard-dirty` | false (disabled) | Alias for --force-pull; resets repo to remote state |
| `--force-pull` | false (disabled) | Reset repos to remote state, losing uncommitted work |
| `--hard-reset` | false (disabled) | Remove logs, configs, and lock files |
| `--list-instances` | false (disabled) | List running instance names and PIDs |
| `--no-hash-check` | false (feature enabled) | Always pull without hash check |
| `--sudo-su` | false (disabled) | Suppress confirmation alerts |
| `--post-pull-hook` |  | Command to execute after successful pull |
| `--confirm-mutant` | false (disabled) | Confirm enabling mutant mode |

### Destructive Actions

These flags permanently alter data and require explicit confirmation:

- `--force-pull` (alias `--discard-dirty`): Resets each repository to match its remote, deleting uncommitted changes and untracked files.
- `--hard-reset`: Wipes autogitpull's log files, configuration files, and lock files under the specified root directory.

## Basics

| Option | Default | Description |
|--------|---------|-------------|
| `--dont-skip-timeouts` | false | Retry repositories that timeout |
| `--help` | false (disabled) | Show this message |
| `--include-dir` |  | Additional directory to scan (repeatable) |
| `--include-private` | false (disabled) | Include private repositories |
| `--interval` | 30 | Delay between scans (s, m, h, d, w, M, Y) |
| `--keep-first-valid` | false (disabled) | Keep valid repos from first scan |
| `--max-depth` | 0 | Limit recursive scan depth |
| `--recursive` | false (disabled) | Scan subdirectories recursively |
| `--refresh-rate` | 250 | TUI refresh rate |
| `--rescan-new` | false (disabled) | Rescan for new repos every N minutes (default 5) |
| `--retry-skipped` | false (disabled) | Retry repositories skipped previously |
| `--reset-skipped` | false (disabled) | Reset status to pending for skipped repos |
| `--dont-skip-unavailable` | false (disabled) | Retry repos missing or invalid on first pass |
| `--root` |  | Root folder of repositories |
| `--single-repo` | false (disabled) | Only monitor the specified root repo |
| `--single-run` | false (disabled) | Run a single scan cycle and exit |
| `--skip-accessible-errors` | false (disabled) | Skip repos with errors even if previously accessible |
| `--updated-since` | 0 | Only sync repos updated recently (m, h, d, w, M, Y) |
| `--wait-empty` | false (disabled) | Keep retrying when no repos are found (optional limit) |

## Authentication

| Option | Default | Description |
|--------|---------|-------------|
| `--ssh-public-key` |  | Path to SSH public key |
| `--ssh-private-key` |  | Path to SSH private key |
| `--credential-file` |  | Read username and password from file |

## Network

| Option | Default | Description |
|--------|---------|-------------|
| `--proxy` |  | HTTP(S) proxy for Git network operations |

## Concurrency

| Option | Default | Description |
|--------|---------|-------------|
| `--concurrency` | 1 | Number of worker threads |
| `--max-threads` | 0 | Cap the scanning worker threads |
| `--single-thread` | 1 | Run using a single worker thread |
| `--threads` | 1 | Alias for --concurrency |

## Config

| Option | Default | Description |
|--------|---------|-------------|
| `--auto-config` | false (disabled) | Auto detect YAML or JSON config |
| `--auto-reload-config` | false (disabled) | Reload config when the file changes (CLI only) |
| `--config-json` |  | Load options from JSON file |
| `--config-yaml` |  | Load options from YAML file |
| `--enable-history` | false (disabled) | Enable command history |
| `--enable-hotkeys` | false (disabled) | Enable TUI hotkeys |
| `--rerun-last` | false (disabled) | Reuse args from .autogitpull.config |
| `--save-args` | false (disabled) | Save args to config file |

## Daemon

| Option | Default | Description |
|--------|---------|-------------|
| `--daemon-config` |  | Config file for daemon install |
| `--daemon-name` | autogitpull | Daemon unit name for install |
| `--daemon-status` | false (disabled) | Check daemon existence and running state |
| `--force-restart-daemon` | false (disabled) | Force restart daemon |
| `--force-stop-daemon` | false (disabled) | Force stop daemon |
| `--install-daemon` | false (disabled) | Install background daemon (systemd/launchd) |
| `--restart-daemon` | false (disabled) | Restart daemon service |
| `--start-daemon` | false (disabled) | Start daemon service |
| `--stop-daemon` | false (disabled) | Stop daemon service |
| `--uninstall-daemon` | false (disabled) | Uninstall background daemon |

## Display

| Option | Default | Description |
|--------|---------|-------------|
| `--censor-char` | '*' | Character for name masking |
| `--censor-names` | false (disabled) | Mask repository names |
| `--color` |  | Override status color |
| `--theme` |  | Load colors from theme file |
| `--color` |  | Override status color |
| `--theme` |  | Load colors from theme file |
| `--hide-date-time` | true (enabled) | Hide date/time line in TUI |
| `--hide-date-time` | true (enabled) | Hide date/time line in TUI |
| `--hide-header` | true (enabled) | Hide status header |
| `--hide-header` | true (enabled) | Hide status header |
| `--no-colors` | false | Disable ANSI colors |
| `--no-colors` | false | Disable ANSI colors |
| `--row-order` | updated | Row ordering (updated/alpha/reverse) |
| `--session-dates-only` | false (disabled) | Only show dates for repos pulled this session |
| `--show-commit-author` | false (disabled) | Display last commit author |
| `--show-commit-author` | false (disabled) | Display last commit author |
| `--show-commit-date` | false (disabled) | Display last commit time |
| `--show-commit-date` | false (disabled) | Display last commit time |
| `--show-notgit` | false (disabled) | Show non-git directories |
| `--show-pull-author` | false (disabled) | Show author when pull succeeds |
| `--show-repo-count` | false (disabled) | Display number of repositories |
| `--show-runtime` | false (disabled) | Display elapsed runtime |
| `--show-skipped` | false (disabled) | Show skipped repositories |
| `--show-version` | false (disabled) | Display program version in TUI |
| `--version` | false (disabled) | Print program version and exit |

## Ignores

| Option | Default | Description |
|--------|---------|-------------|
| `--add-ignore` | false (disabled) | Add path to .autogitpull.ignore |
| `--clear-ignores` | false (disabled) | Delete all ignore entries |
| `--depth` | 2 | Depth for --find-ignores/--clear-ignores |
| `--find-ignores` | false (disabled) | List ignore entries |
| `--ignore` |  | Directory to ignore (repeatable) |
| `--remove-ignore` | false (disabled) | Remove path from ignore file |

## Kill

| Option | Default | Description |
|--------|---------|-------------|
| `--kill-all` | false (disabled) | Terminate running instance and exit |
| `--kill-on-sleep` | false (disabled) | Exit if a system sleep is detected |

## Lock

| Option | Default | Description |
|--------|---------|-------------|
| `--ignore-lock` | false (disabled) | Don't create or check lock file |
| `--remove-lock` | false (disabled) | Remove directory lock file and exit |

## Logging

| Option | Default | Description |
|--------|---------|-------------|
| `--debug-memory` | false (disabled) | Log memory usage each scan |
| `--dump-large` | 0 | Dump threshold for --dump-state |
| `--dump-state` | false (disabled) | Dump container state when large |
| `--log-dir` |  | Directory for pull logs |
| `--log-file` |  | File for general logs |
| `--log-level` | INFO | Set log verbosity |
| `--max-log-size` | 0 | Rotate --log-file when over this size |
| `--json-log` | false (disabled) | Emit logs in JSON format |
| `--compress-logs` | false (disabled) | Gzip rotated log files |
| `--syslog` | false (disabled) | Log to syslog |
| `--syslog-facility` | 0 | Syslog facility |
| `--verbose` | INFO | Shorthand for --log-level DEBUG |

## Process

| Option | Default | Description |
|--------|---------|-------------|
| `--attach` |  | Attach to daemon and show status |
| `--background` | false (disabled) | Run in background with attach name |
| `--cli` | false (disabled) | Use console output |
| `--exit-on-timeout` | false (disabled) | Terminate worker on poll timeout |
| `--exit-on-timeout` | false (disabled) | Terminate worker on poll timeout |
| `--keep-first` | false (disabled) | Keep repos validated on first scan |
| `--max-runtime` | 0 | Exit after given runtime (s, m, h, d, w, M, Y) |
| `--persist` | false (disabled) | Keep running after exit (optional name) |
| `--print-skipped` | false (disabled) | Print skipped repositories once |
| `--pull-timeout` | 0 | Network operation timeout (s, m, h, d, w, M, Y) |
| `--mutant` | false (disabled) | Enable full auto mutant mode with smart verification and adaptive timeouts |
| `--recover-mutant` | false (disabled) | Recover persisted mutant session |
| `--mutant-config` |  | Path to mutant config file |
| `--pull-timeout` | 0 | Network operation timeout (s, m, h, d, w, M, Y) |
| `--reattach` | false (disabled) | Reattach to background process |
| `--respawn-limit` | 0 | Respawn limit within minutes |
| `--respawn-delay` | 1s | Delay between worker respawns (ms, s, m, h, d, w, M, Y) |
| `--silent` | false (disabled) | Disable console output |

## Resource limits

| Option | Default | Description |
|--------|---------|-------------|
| `--cpu-cores` | 0 | Set CPU affinity mask |
| `--cpu-percent` | 0.0 | Approximate CPU usage limit |
| `--disk-limit` | 0 | Limit disk throughput |
| `--download-limit` | 0 | Limit total download rate |
| `--mem-limit` | 0 | Abort if memory exceeds this amount |
| `--total-traffic-limit` | 0 | Stop after this much traffic |
| `--upload-limit` | 0 | Limit total upload rate |

## Service

| Option | Default | Description |
|--------|---------|-------------|
| `--force-restart-service` | false (disabled) | Force restart service |
| `--force-stop-service` | false (disabled) | Force stop service |
| `--install-service` | false (disabled) | Install system service (launchd/systemd/Windows) |
| `--list-daemons` | false (disabled) | Alias for --list-services |
| `--list-services` | false (disabled) | List installed service units |
| `--restart-service` | false (disabled) | Restart service |
| `--service-config` |  | Config file for service install |
| `--service-name` | autogitpull | Service name for install |
| `--service-status` | false (disabled) | Check service existence and running state |
| `--show-service` | false (disabled) | Show installed service name |
| `--start-service` | false (disabled) | Start installed service |
| `--stop-service` | false (disabled) | Stop installed service |
| `--uninstall-service` | false (disabled) | Uninstall system service |

## Tracking

| Option | Default | Description |
|--------|---------|-------------|
| `--cpu-poll` | 5 | CPU usage polling interval (s, m, h, d, w, M, Y) |
| `--mem-poll` | 5 | Memory usage polling interval (s, m, h, d, w, M, Y) |
| `--net-tracker` | false (disabled) | Track network usage |
| `--no-cpu-tracker` | false (feature enabled) | Disable CPU usage tracker |
| `--no-mem-tracker` | false (feature enabled) | Disable memory usage tracker |
| `--no-thread-tracker` | false (feature enabled) | Disable thread tracker |
| `--thread-poll` | 5 | Thread count polling interval (s, m, h, d, w, M, Y) |
| `--vmem` | false (disabled) | Show virtual memory usage |
