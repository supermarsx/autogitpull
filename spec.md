# autogitpull — Product & Engineering Spec

> Automatic Git Puller & Monitor; cross‑platform CLI/TUI for scanning a root folder of Git repositories, pulling on a schedule, and optionally running as a background task with attach/reattach.

---

## 1) Problem & Goals

**Problem.** Developers with dozens of local repos waste time pulling changes and monitoring status across projects. Servers/lab machines need an unattended way to keep checkouts up to date without losing local work, while staying resource‑friendly.

**Goals.**

- Scan a root directory (optionally recursive) for git repositories and continuously keep them in sync with upstream via `git pull`.
- Perform all Git operations through libgit2 rather than invoking the external `git` CLI.
- Offer both an interactive TUI dashboard and a plain CLI log mode.
- Provide robust scheduling, resource limiting (CPU/memory/network/disk), and background persistence with attach/reattach.
- Be safe by default (skip dirty repos), with explicit overrides (`force pull`) guarded by confirmation options.
- Be configuration‑driven (YAML/JSON) and transparent via structured logs.

**Non‑Goals.**

- No write operations beyond safe pulls/reset operations (no merges/rebases/commits/pushes).
- Not a CI/CD system; it only syncs checkouts.

---

## 2) Target Users & Use Cases

- **Poly‑repo developer workstations.** Keep many working copies current; quickly see which repos updated during the session.
- **Build agents/CI executors (read‑only sync).** Maintain clean mirrors of upstream for build/test.
- **Lab machines/kiosks/classrooms.** Pull latest examples/content on interval with resource caps.
- **Air‑gapped/limited‑bandwidth environments.** Constrain total network/disk throughput and session runtime.

---

## 3) High‑Level Design

### 3.1 Components

- **Scanner.** Walks the root and `include_dir`s, filters ignored paths, detects Git working trees, optionally recursive with depth limit.

- **Repo Worker.** For each repo, resolves remotes/branches and performs fetch/pull via libgit2 (or configurable reset path), honoring timeouts and resource caps, recording per‑repo metrics and logs.

- **Scheduler.** Drives scan cycles on a configurable `interval`; supports `single-run` and `updated-since` filtering; can rescan the filesystem for new repos (`rescan-new`).

- **Resource Trackers.** Periodically sample CPU, memory, thread count, and optionally network I/O; enforce user caps (`cpu-percent`, `mem-limit`, `download-limit`, `upload-limit`, `disk-limit`, `total-traffic-limit`).

- **TUI Layer.** Interactive dashboard with periodic `refresh-rate`, sortable rows, optional column toggles (commit author/date, repo count, runtime), ANSI color control and censorship mode to mask repo names.

- **CLI Output.** Plain log stream for piping to files/syslog or running in `--cli` mode without TUI.

- **Background Daemonizer.** Run in background (`--background`/`--persist`) and expose an **attach** channel (`--attach`/`--reattach`), with restart controls (`--respawn-limit`, `--max-runtime`).

- **Config Loader.** Load flags from YAML/JSON (`--config-yaml`, `--config-json`), auto‑detect (`--auto-config`), and auto‑reload.

- **Logging.** File and syslog sinks, rotation (`--max-log-size`), severity control, optional dumps (`--dump-state`, `--dump-large`).

- **MCP Server (optional; disabled by default).** Implements a Model Context Protocol server over stdio/TCP/Unix/WS/HTTP, exposing read‑only tools and resources for observability and safe control; destructive tools are gated behind explicit flags and confirmations.

### 3.2 Data/Runtime Model

- **Working set.** In‑memory container of discovered repos with status (clean/dirty/skipped), last pull result, last commit meta (author/time), byte counters, and throttling state.
- **History.** Optional persisted arg history (`--enable-history`) via a dotfile to quickly rerun last command (`--rerun-last`, `--save-args`).

### 3.3 Git Operations & Safety

- Default: **skip dirty repos** to avoid data loss; print them once (`--print-skipped`).
- Override: `--force-pull` (alias `--discard-dirty`) to reset local changes to upstream; gated behind `--confirm-reset`/`--confirm-alert` for explicit acknowledgment.
- Timeout handling: per‑operation `--pull-timeout` with optional `--exit-on-timeout` (worker termination) and `--dont-skip-timeouts` for retry policy.

---

## 4) CLI Contract (abbreviated)

> The binary accepts a rich set of flags; below is a condensed mapping grouped by concern. (See **Appendix A** for a fuller table.)

**Basics**

- `--root <path>` / `--include-dir <dir>` / `--ignore <dir>` / `--recursive` / `--max-depth <n>`
- `--interval <Ns|m|h|d|w|M|Y>` / `--single-run` / `--updated-since <Nh|d|...>` / `--rescan-new <min>` / `--wait-empty [n]`

**Display**

- `--cli` / `--refresh-rate <ms|s|m>` / `--show-*` toggles (skipped/notgit/version/repo-count/commit-date/commit-author/pull-author) / `--row-order <alpha|reverse>` / `--color <ansi>` / `--no-colors` / `--censor-names [--censor-char]`

**Process/Daemon**

- `--background <name>` / `--attach <name>` / `--reattach <name>` / `--persist[=name]` / `--respawn-limit <n[,min]>` / `--max-runtime <N...>` / `--silent`

**Concurrency & Limits**

- `--threads|--concurrency <n>` / `--single-thread` / `--max-threads <n>`
- `--cpu-percent <n.n>` / `--cpu-cores <mask>` / `--mem-limit <MiB|GiB>`
- `--download-limit <KB|MB>` / `--upload-limit <KB|MB>` / `--disk-limit <KB|MB>` / `--total-traffic-limit <KB|MB|GB>`

**Tracking**

- `--cpu-poll|--mem-poll|--thread-poll <Ns|m|...>` / `--no-*-tracker` / `--net-tracker` / `--vmem`

**Logging**

- `--log-dir <path>` / `--log-file <file>` / `--max-log-size <bytes>` / `--log-level <level>` / `--verbose` / `--syslog[ --syslog-facility <n>]` / debugging dumps

**Safety & Git Behavior**

- `--check-only` / `--no-hash-check` / `--force-pull` / `--remove-lock` / `--hard-reset` / `--confirm-reset` / `--confirm-alert` / `--pull-timeout <N...>` / `--exit-on-timeout` / `--dont-skip-timeouts`

**MCP Server (optional; disabled by default)**

- **Enable/Mode**: `--mcp-enable` (off by default), `--mcp-mode <readonly|admin>` (default `readonly`), `--mcp-allow-destructive` (off) + `--confirm-mcp-destructive` (required if enabling destructive tools).
- **Transport**: `--mcp-transport <stdio|tcp|unix|ws|http>` (default `stdio` when enabled); aliases: `--mcp-stdio`, or bind options: `--mcp-bind <host:port>`, `--mcp-host <host>`, `--mcp-port <port>`, `--mcp-unix-socket <path>`, `--mcp-ws-path </path>` (default `/mcp`), `--mcp-http-path </path>` (default `/mcp`).
- **Auth/TLS**: `--mcp-auth <none|bearer|basic|mtls>` (default `none` for `stdio`, `bearer` recommended for networked), `--mcp-bearer-token-env <VAR>` / `--mcp-bearer-token-file <path>`, `--mcp-basic-user <user>` + (`--mcp-basic-pass-env <VAR>` | `--mcp-basic-pass-file <path>`), TLS: `--mcp-tls-cert <path>` / `--mcp-tls-key <path>` / `--mcp-tls-ca <path>` / `--mcp-tls-require`, mTLS: `--mcp-mtls-required`, `--mcp-mtls-ca <path>`.
- **Exposure controls**: `--mcp-allow-tools <csv>` / `--mcp-deny-tools <csv>`; repo scoping: repeatable `--mcp-repo-include <glob>` / `--mcp-repo-exclude <glob>`. Name/URL privacy overrides: `--mcp-censor-names`, `--mcp-redact-urls`.
- **Limits**: `--mcp-max-connections <n>`, `--mcp-max-concurrent-tools <n>`, `--mcp-rate-limit-rps <n>`, `--mcp-rate-burst <n>`, `--mcp-max-request-bytes <n>`, `--mcp-max-response-bytes <n>`, `--mcp-tool-timeout <dur>`, `--mcp-idle-timeout <dur>`, `--mcp-keepalive <dur>`, `--mcp-event-buffer <n>`.
- **Logging/Tracing**: `--mcp-access-log <path|stdout|off>`, `--mcp-log-requests`, `--mcp-log-payloads` (payload fields redacted by default), `--mcp-log-redact-secrets` (on by default), `--mcp-trace <off|basic|verbose>`.
- **Identity/Session**: `--mcp-id <name>` (instance identity), `--mcp-session-name <name>`, `--mcp-allow-origins <csv>` (for WS/HTTP CORS).

---

## 5) Configuration

Two equivalent formats are supported: **YAML** and **JSON**. The loader supports: `--config-yaml`, `--config-json`, `--auto-config`, `--auto-reload-config`, `--enable-history`, `--save-args`, `--rerun-last`.

### 5.1 Example — YAML

```yaml
# autogitpull.yaml
root: "/work/repos"                    # required root folder
include_private: false
recursive: true
max_depth: 4
include_dir:
  - "/opt/extra-repos"
ignore:
  - "/work/repos/tmp"
interval: "15m"
updated_since: "6h"                     # only sync repos updated recently
rescan_new: 10                           # minutes
wait_empty: 3                            # retries when no repos found
threads: 8
single_thread: false
cpu_percent: 35.0
cpu_cores: "0-7"
mem_limit: "4G"
download_limit: "5MB"
upload_limit: "1MB"
disk_limit: "10MB"
total_traffic_limit: "2GB"
log_dir: "/var/log/autogitpull/pulls"
log_file: "/var/log/autogitpull/agent.log"
max_log_size: 10485760
log_level: "INFO"
cli: false
refresh_rate: "500ms"
show_skipped: true
show_commit_date: true
show_commit_author: true
censor_names: false
background: true
persist: "main"
attach: null
pull_timeout: "90s"
exit_on_timeout: false
check_only: false
force_pull: false
hard_reset: false
confirm_reset: false
confirm_alert: false
session_dates_only: true
row_order: "alpha"
```

> JSON mirrors the same keys; CLI flags always take precedence over config file values.

### 5.2 Example — MCP (YAML addendum)

```yaml
mcp:
  enable: false                # disabled by default
  mode: "readonly"            # or "admin" (requires allow-destructive + confirm)
  transport: "stdio"          # stdio|tcp|unix|ws|http (only used if enable=true)
  bind: "127.0.0.1:8765"      # for tcp/ws/http (ignored for stdio/unix)
  unix_socket: null            # e.g., "/tmp/autogitpull.mcp.sock"
  ws_path: "/mcp"
  http_path: "/mcp"
  auth: "none"                # none|bearer|basic|mtls
  bearer_token_env: "AUTOGITPULL_MCP_TOKEN"
  bearer_token_file: null
  basic_user: null
  basic_pass_env: null
  basic_pass_file: null
  tls_cert: null
  tls_key: null
  tls_ca: null
  tls_require: false
  mtls_required: false
  mtls_ca: null
  allow_tools: ["listRepos","getRepoStatus","scanRepos","streamEvents"]
  deny_tools: []
  repo_include: ["/work/repos/*"]
  repo_exclude: ["**/tmp/**"]
  censor_names: true
  redact_urls: true
  max_connections: 8
  max_concurrent_tools: 4
  rate_limit_rps: 20
  rate_burst: 40
  max_request_bytes: 1048576
  max_response_bytes: 8388608
  tool_timeout: "30s"
  idle_timeout: "2m"
  keepalive: "30s"
  event_buffer: 1024
  access_log: "stdout"
  log_requests: false
  log_payloads: false
  log_redact_secrets: true
  trace: "off"                 # off|basic|verbose
  id: "autogitpull-mcp"
  session_name: null
  allow_origins: ["http://localhost:3000"]
```

> JSON mirrors this `mcp` object and can be nested alongside the main configuration.

---

## 6) UX Behavior

### 6.1 TUI (interactive)

- **Header:** runtime, repo count, interval, last scan window, throttle state.
- **Rows:** name (masked if `censor-names`), status (OK/UPDATED/SKIPPED/DIRTY/TIMEOUT/ERROR), last commit author/time (if enabled), bytes in/out (if `net-tracker`), elapsed time.
- **Sort/refresh:** periodic update via `refresh-rate`, with deterministic row order control.

### 6.2 CLI (log)

- Line‑oriented messages with ISO timestamps, severity, repo path (or masked), humanized deltas and transfer sizes.

---

## 7) Build & Packaging

### 7.1 Repository layout (observed)

- `CMakeLists.txt`, `Makefile` — build system entry points.
- `src/`, `include/` — implementation and headers.
- `tests/` — test sources.
- `docs/` — CLI docs and ancillary documentation.
- `scripts/` — helper scripts.
- `manifest/` — metadata/manifests (e.g., icons, app metadata).
- `.clang-format`, `CPPLINT.cfg` — code style and linting configuration.

### 7.2 Build targets

- **Minimum toolchain (suggested):** CMake ≥ 3.20; a modern C++ compiler (GCC/Clang/MSVC). Follow `.clang-format` and `CPPLINT.cfg`.
- **Runtime dependency:** Git CLI present in `PATH`.

#### Unix/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
# Optionally
cmake --install build --prefix /usr/local
```

#### Windows (MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -m
```

> If `Makefile` provides convenience targets, they should wrap the CMake flow; prefer invoking CMake directly in automation/CI.

### 7.3 Artifacts

- Single executable `autogitpull` (+ optional symbol files). Include icon metadata; embed version for `--version`.

---

## 8) Deployment & Service Wrappers

Although the binary supports background/persist/attach, provide optional OS service wrappers for ops teams:

**systemd (Linux)**

```ini
# /etc/systemd/system/autogitpull.service
[Unit]
Description=Automatic Git Puller & Monitor
After=network-online.target

[Service]
Type=simple
ExecStart=/usr/local/bin/autogitpull --root /srv/repos --interval 10m --recursive --background svc --persist svc --log-file /var/log/autogitpull/agent.log --log-dir /var/log/autogitpull/pulls --log-level INFO
Restart=on-failure
RestartSec=15
User=autogit
Group=autogit

[Install]
WantedBy=multi-user.target
```

**launchd (macOS)** — provide a `.plist` with `KeepAlive` and `StandardOutPath`/`StandardErrorPath`.

**Windows Task Scheduler** — XML export pointing to `autogitpull.exe` with arguments and `Run whether user is logged on or not`.

---

## 9) Security Considerations

- **Principle of Least Surprise.** Default to **non‑destructive** behavior: skip dirty repos; show one‑time warning (`--print-skipped`).
- **Dangerous operations require confirmation.** `--force-pull`/`--hard-reset` must require `--confirm-reset` or `--confirm-alert`. Consider a 3‑second negative confirmation window in TUI before executing destructive actions.
- **Credential handling.** Do **not** implement custom secret storage. Rely on Git’s credential helpers (OS keychain, manager‑core). Never echo credentials to logs. Respect `GIT_ASKPASS`/`SSH_AUTH_SOCK`.
- **Path safety.** Treat repo paths as untrusted input; avoid shell interpolation; use exec with argv to call `git` directly; prohibit path traversal outside the configured roots.
- **Command allowlist.** Only execute `git` subcommands required for sync: `git rev-parse`, `git status --porcelain`, `git fetch`, `git pull`, optional `git reset --hard`, `git gc --auto`.
- **Resource abuse controls.** Enforce user caps and `respawn-limit`; cap worker concurrency per CPU.
- **Log redaction.** Respect `--censor-names` and ensure no URLs with embedded creds are logged; strip `https://user:pass@host` forms.
- **File permissions.** Create logs with `0600` on Unix; prefer app‑private directories on Windows (`CSIDL_LOCAL_APPDATA`).
- **Symlink & submodule handling.** Resolve symlinks safely; treat submodules explicitly (see §11); never follow unbounded symlink loops.
- **Update channel.** If auto‑update is ever introduced, sign binaries and verify signatures; not in scope for this release.

---

### 9.1 MCP Server — Additional Security

- **Default‑off posture.** The MCP server is **disabled by default** and must be explicitly enabled with `--mcp-enable` or `mcp.enable: true`.
- **Local‑only by default.** When enabled without bind flags, prefer `stdio` transport. If a network transport is selected, default bind is `127.0.0.1`.
- **Least privilege.** Default mode is `readonly`; destructive tools require `--mcp-allow-destructive` **and** `--confirm-mcp-destructive` in the same invocation.
- **Auth required for network transports.** Enforce `--mcp-auth bearer` or `mtls` when using TCP/WS/HTTP; reject `none` on non‑loopback binds.
- **Redaction.** Always redact secrets and credential‑bearing URLs in logs and responses when `--mcp-log-payloads` is set.
- **Repo scoping.** Respect `repo_include`/`repo_exclude` to prevent data exfiltration outside approved paths.
- **Rate limiting & timeouts.** Apply global and per‑tool limits; drop clients exceeding thresholds.
- **Input validation.** Strict JSON schema validation for all tool calls; reject unknown fields by default.

## 10) Performance & Reliability

- **Targets.**
  - 5k repos scanned in ≤ 20s on 8‑core/16‑thread desktop with SSD when `updated-since` is used.
  - Memory footprint < 150 MiB at steady state with trackers enabled.
  - CPU default budget ≤ 25% of one core; all caps respected.
- **Backoff & retries.** Exponential backoff for transient network errors up to a limit; classify timeouts vs. hard failures.
- **Resilience.** Process restart with bounded `respawn-limit`; queue cleanup on crash with `--remove-lock`.

---

## 11) Edge Cases & Git Nuances

- **Dirty/unstaged work.** Always skipped unless explicitly overridden; surface a summary table at the end of a cycle.
- **Diverged branches.** If `pull` cannot fast‑forward, mark repo as `CONFLICT` and skip until user intervention (don’t attempt merge).
- **Submodules.** Expose `--submodules <ignore|init|update>` in a future minor; for now, document behavior (current release: treat as normal working tree and do not recurse).
- **Shallow clones.** Allow pulls but warn on depth‑related fetch failures.
- **LFS.** Don’t force `git lfs fetch`; respect user’s global Git config.

---

## 12) Testing Strategy

### 12.1 Unit Tests

- Scanner logic (recursive detection, ignore logic, max‑depth enforcement).
- Config parsing/precedence between YAML/JSON/flags; auto‑reload behavior.
- Rate limiter and resource tracker samplers.
- Git command orchestrator (mocked `git` executable) and timeout paths.

### 12.2 Integration Tests

- **Temp repo matrix:** bare+worktree; clean vs. dirty; diverged vs. fast‑forward; submodules; shallow clones; LFS.
- **Concurrency:** stress with 1, 2, N threads; verify determinism of row ordering and aggregate limits.
- **Limits:** enforce `cpu-percent`, `mem-limit`, and each I/O limiter; assert early abort on breaches.
- **Background/attach:** run with `--persist` and attach/reattach sequences; verify `respawn-limit` behavior.
- **Logging:** size‑based rotation; syslog emission; redaction of URLs/usernames; `--censor-names` masks.
- **Timeouts:** simulate slow network and trigger `--pull-timeout`; ensure retry/skip policies are applied.

### 12.3 TUI/CLI Snapshot Tests

- Run in `--cli` with deterministic settings and record golden outputs; compare cleanly across platforms.

### 12.5 MCP Server Tests

- **Handshake/Capabilities:** validate MCP handshake and advertised tools/resources match CLI/config gating.
- **Auth paths:** bearer/basic/mTLS happy paths and rejection cases; token reload from env/file; loopback without auth for stdio only.
- **Rate limits/timeouts:** simulate abusive clients; ensure back‑pressure and disconnects occur per limits.
- **Schema/property tests:** fuzz JSON payloads for each tool; ensure strict validation and stable error codes.
- **Permissions:** verify `readonly` blocks destructive tools; enabling requires both flags; repo include/exclude enforced.
- **Transport matrix:** stdio, tcp (127.0.0.1), unix socket, ws/http; TLS and mTLS setups where applicable.
- **Observability:** request/response logging redactions; access log formatting; event streaming buffer overflow handling.
- Run in `--cli` with deterministic settings and record golden outputs; compare cleanly across platforms.

### 12.4 Tooling & CI

- **Static analysis:** cpplint (repo cfg), clang‑tidy, `-Wall -Wextra -Werror`.
- **Sanitizers:** ASan/UBSan/MSan builds in CI nightly.
- **Matrix:** Ubuntu (gcc/clang), macOS (clang), Windows (MSVC), release+debug, with ccache if available.
- **CTest** wired into CMake if applicable; otherwise bespoke test runner in `tests/`.

---

## 13) Telemetry & Observability

- Structured JSON logs (optional) for easier ingestion.
- Per‑cycle summary: repos scanned, updated, skipped (dirty/notgit/timeout), bytes transfer, peak RSS, CPU budget adherence.

---

- **MCP metrics/logging:** include per‑client connection counts, tool invocation counts/durations/success rates, rate‑limit drops, max queue depth, and last error per client; report in structured logs and optional summary.

## 14) Documentation & Examples

- **Quick Start** (README): one‑liner `autogitpull /path/to/repos --interval 10m --recursive` with safety caveats.
- **Examples/**: sample YAML+JSON configs; service files for systemd/launchd/scheduler.
- **docs/**: complete CLI reference; troubleshooting (timeouts, auth prompts, diverged branches).

---

## 15) Roadmap (post‑MVP)

- Submodule management flag.

- Optional Git CLI fallback for environments where libgit2 is unavailable.

- Plugin hook for post‑pull actions (read‑only, e.g., notify or indexer touch).

- Export metrics endpoint (Prometheus textfile or HTTP).

- Cross‑platform packaging (Homebrew, Winget, apt tap, Scoop).

- **MCP enhancements:** dynamic tool discovery, per‑client policies, OpenTelemetry spans, and signed client manifests.

- Submodule management flag.

- Optional Git CLI fallback for environments where libgit2 is unavailable.

- Plugin hook for post‑pull actions (read‑only, e.g., notify or indexer touch).

- Export metrics endpoint (Prometheus textfile or HTTP).

- Cross‑platform packaging (Homebrew, Winget, apt tap, Scoop).

---

## 16) Acceptance Criteria (MVP)

1. Runs on macOS, Ubuntu, and Windows; discovers repos and performs safe pulls on schedule.
2. Default behavior never loses uncommitted changes; destructive actions guarded by explicit flags.
3. TUI updates at configured rate and shows at least: repo name, status, last commit author/time, transfer totals; CLI mirrors events with timestamps.
4. Background/persist mode works; `--attach` returns a live view; `--reattach` recovers if client detaches; respects respawn limit.
5. All documented limits and trackers function; logs rotate and respect size caps.
6. Configuration via YAML or JSON works with auto‑detect; CLI flags override config.
7. Test suite covers scanner, config precedence, concurrency, timeout handling, and logging.

---

## 17) MCP Server Extension (Detailed)

### 17.1 Capabilities & Surfaces

- **Tools** (JSON‑RPC over MCP):

  - `listRepos()` → array of repositories with `name`, `path`, `status`, `lastCommitAuthor`, `lastCommitTime`, `dirty`, `bytesIn`, `bytesOut`.
  - `scanRepos({ rescanNew?: boolean })` → `{ reposFound, durationMs }`.
  - `getRepoStatus({ path })` → full repo state snapshot.
  - `pullRepo({ path, timeoutMs? })` → `{ updated: boolean, bytesIn, bytesOut, durationMs }` (readonly mode allows safe pull only if repo is clean and fast‑forward).
  - `pullAll({ parallel?: number, updatedSince?: string })` → per‑repo results.
  - `forcePullRepo({ path, confirm: "I_UNDERSTAND", timeoutMs? })` → **admin only**, performs hard reset + pull.
  - `tailLogs({ repoPath?, lines?: number })` → recent log lines.
  - `streamEvents({})` → server‑sent event stream: repo updates, warnings, errors.
  - `getConfig()` / `setConfig({ patch })` → read/patch runtime config (fields gated by mode).
  - `attachSession({ name })` → attaches to background session (read‑only stream if not admin).
  - `ping({})` → liveness check.

- **Resources**: read‑only blobs for

  - `resource://summary` (cycle summary JSON),
  - `resource://repo/<hash>` (sanitized status JSON),
  - `resource://logs/<date>` (rotated log access subject to redaction).

### 17.2 JSON Schemas (abridged)

```json
// pullRepo input
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": { "path": {"type": "string"}, "timeoutMs": {"type": "integer", "minimum": 1} },
  "required": ["path"],
  "additionalProperties": false
}
```

```json
// forcePullRepo input (admin only)
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "path": {"type": "string"},
    "timeoutMs": {"type": "integer", "minimum": 1},
    "confirm": {"type": "string", "pattern": "^I_UNDERSTAND$"}
  },
  "required": ["path", "confirm"],
  "additionalProperties": false
}
```

### 17.3 Defaults & Safe Behavior

- Server disabled; when enabled defaults to **stdio**, **readonly**, and **loopback‑only** for network transports.
- Tools exposed by default: `listRepos`, `getRepoStatus`, `scanRepos`, `streamEvents`.
- Admin‑only tools (`pullAll` with force, `forcePullRepo`, `setConfig` on destructive keys) require both flag gating and runtime confirmation.

### 17.4 Failure Modes & Errors

- Standardized error codes: `MCP_UNAUTHORIZED`, `MCP_FORBIDDEN`, `MCP_RATE_LIMITED`, `MCP_TIMEOUT`, `MCP_INVALID_INPUT`, `MCP_INTERNAL`.
- Include `retryAfterMs` where applicable.

### 17.5 Operational Notes

- For WS/HTTP transports, default CORS allowlist is empty; must be explicitly configured.
- Access logs include client id, transport, tool, status code, duration, bytes, redaction applied.

---

## Appendix A — Representative CLI Matrix

| Concern     | Key Flags                                                                                                                                                                               |                                                     |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| Discovery   | `--root`, `--include-dir`, `--ignore`, `--recursive`, `--max-depth`, `--rescan-new`, `--wait-empty`                                                                                     |                                                     |
| Scheduling  | `--interval`, `--single-run`, `--updated-since`                                                                                                                                         |                                                     |
| Display     | `--refresh-rate`, `--show-*`, `--row-order`, `--color`, `--no-colors`, `--censor-names`                                                                                                 |                                                     |
| Process     | `--background`, `--attach`, `--reattach`, `--persist`, `--respawn-limit`, `--max-runtime`, `--silent`                                                                                   |                                                     |
| Concurrency | \`--threads                                                                                                                                                                             | --concurrency`, `--single-thread`, `--max-threads\` |
| Limits      | `--cpu-percent`, `--cpu-cores`, `--mem-limit`, `--download-limit`, `--upload-limit`, `--disk-limit`, `--total-traffic-limit`                                                            |                                                     |
| Tracking    | `--cpu-poll`, `--mem-poll`, `--thread-poll`, `--no-*-tracker`, `--net-tracker`, `--vmem`                                                                                                |                                                     |
| Logging     | `--log-dir`, `--log-file`, `--max-log-size`, `--log-level`, `--verbose`, `--syslog`, `--syslog-facility`                                                                                |                                                     |
| Safety      | `--check-only`, `--no-hash-check`, `--force-pull`, `--remove-lock`, `--hard-reset`, `--confirm-reset`, `--confirm-alert`, `--pull-timeout`, `--exit-on-timeout`, `--dont-skip-timeouts` |                                                     |

---

## Appendix B — Example Troubleshooting

- **Auth prompt appears unexpectedly.** Ensure your Git credential helper is configured (e.g., `git-credential-manager` on Windows/macOS) and that SSH agent is running if using SSH URLs.
- **Repos show TIMEOUT.** Increase `--pull-timeout` or reduce `--threads`; verify network connectivity and remote availability.
- **Dirty repos always skipped.** Use `--force-pull --confirm-reset` if you truly intend to discard work; otherwise manually commit/stash.
- **High CPU usage.** Lower `--threads`, raise `--interval`, or set `--cpu-percent` cap; disable trackers if not needed.
- **Attach fails.** Ensure the background name matches; confirm the process was launched with `--background` or `--persist`.

