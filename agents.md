# Contribution Guidelines — autogitpull

> Authoritative contributor policy for building, testing, reviewing, and releasing **autogitpull**. All rules are mandatory unless explicitly marked “optional”.

---

## 0. Scope & Principles

- **Safety-first:** Default behavior must never destroy user work. Do not introduce merges/rebases/commits/pushes. Destructive paths (`--force-pull`, `--hard-reset`) always require explicit confirmations.
- **libgit2, not Git CLI:** Implement sync using libgit2 APIs; avoid spawning external `git` subprocesses.
- **Transparency:** Prefer structured logs, deterministic TUI/CLI output, and clear error codes.
- **Cross‑platform:** Linux, macOS, Windows are first-class.

---

## 1. Toolchain Requirements

- **Build system:** CMake ≥ 3.20 (single source of truth). A Makefile may wrap CMake but must not diverge.
- **Compilers:** GCC/Clang/MSVC capable of **C++17 or later** (C++20 preferred when available).
- **Runtime dependency:** `libgit2` available to the build system.
- **Host tools (mandatory):** `clang-format`, `cpplint`.
- **Host tools (recommended):** `clang-tidy`, `ccache`.
- **Windows:** Visual Studio 2022 or newer; Developer PowerShell.

Set environment variables consistently (examples): `CCACHE_DIR`, `CXXFLAGS`, `LDFLAGS`.

---

## 2. Repository Layout

```
/                       # root
├─ CMakeLists.txt       # canonical build entry
├─ Makefile             # wraps CMake targets (optional convenience)
├─ src/                 # implementation
├─ include/             # headers (1:1 with src/*)
├─ tests/               # unit + integration tests (Catch2); CTest driver
├─ docs/                # CLI/TUI docs & troubleshooting
├─ examples/            # sample YAML/JSON configs and service files
├─ scripts/             # helper scripts (must call into CMake)
├─ manifest/            # packaging metadata (icons, manifests)
├─ .clang-format, CPPLINT.cfg, .clang-tidy (optional)
└─ .gitattributes, .editorconfig
```

---

## 3. Build Instructions

### 3.1 Unix/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
# optional install
cmake --install build --prefix /usr/local
```

### 3.2 Windows (MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -m
```

**Do not** maintain ad‑hoc compiler scripts. If `scripts/compile*.{sh,bat}` exist, they must shell out to CMake and remain no‑logic wrappers.

---

## 4. Formatting, Linting, Static Analysis

- Run locally **before** commit:

```bash
make format   # or: clang-format -i $(git ls-files "*.{c,cc,cpp,h,hpp}")
make lint     # cpplint using repo CPPLINT.cfg
make tidy     # optional; clang-tidy via CMake preset or script
```

- Required compiler flags in CMake: `-Wall -Wextra -Werror` (or MSVC equivalents). Do not suppress warnings globally; scope them narrowly with comments.
- No functional diffs in formatting-only commits.

---

## 5. Testing Policy (must pass locally and in CI)

### 5.1 Unit Tests

- Coverage: scanner (walk/ignore/max-depth), config loader (YAML/JSON + precedence; auto‑reload), rate limiters/trackers, git orchestrator (libgit2 interactions mocked), timeout handling.
- Framework: Catch2 + CTest. Each test must be deterministic and hermetic.

### 5.2 Integration Tests

- Matrix: bare vs. worktree, clean/dirty, diverged vs. fast‑forward, shallow clones, LFS presence, submodules (documented as non‑recurse for MVP), concurrency (1/2/N), resource limit enforcement, background/attach/reattach, logging rotation and redaction, timeout paths.
- Create ephemeral temp repos; never mutate developer’s working tree.

### 5.3 TUI/CLI Snapshot Tests

- Run in `--cli` mode with deterministic settings; verify golden outputs across platforms.

### 5.4 MCP Server Tests (when enabled)

- Handshake/capabilities; auth (bearer/basic/mTLS); rate limits & timeouts; strict JSON schema; permissions (readonly vs. admin); transport matrix; redaction; event buffer overflow behavior.

### 5.5 Sanitizers & Reliability

- Nightly CI jobs: ASan/UBSan/MSan builds.
- Exponential backoff verification for transient network failures.

### 5.6 Commands

```bash
make test              # developer entrypoint
ctest --test-dir build -j
```

---

## 6. Runtime Behavior Requirements (enforced by code review)

- **Safety:** Skip dirty repos by default; `--print-skipped` once per cycle. Fast‑forward only; mark diverged as `CONFLICT` and skip.
- **Timeouts:** Per‑op `--pull-timeout`; support `--exit-on-timeout` and `--dont-skip-timeouts` behaviors.
- **Resource caps:** Implement CPU/mem/net/disk/total traffic limits; never exceed user budgets.
- **Backgrounding:** `--background/--persist` must expose attach/reattach; honor `--respawn-limit`, `--max-runtime`.
- **Config:** YAML/JSON parity; CLI flags override config; support `--auto-config` & auto‑reload.
- **Logging:** Rotation by size; syslog optional; redact secrets and URLs; respect `--censor-names`.
- **TUI:** Refresh at `--refresh-rate`; sortable rows; optional columns (commit author/date, repo count, runtime); ANSI color control.

---

## 7. Security & Privacy Requirements

- Never implement custom secret storage; rely on Git credential helpers. Never log credentials; strip `https://user:pass@host` forms.
- Treat repo paths as untrusted; avoid shell interpolation; interact solely through libgit2 (no `git` subprocesses).
- Resolve symlinks safely; avoid traversal outside configured roots.
- MCP server **disabled by default**. If enabled, default to stdio, readonly, loopback‑only; network transports require auth; destructive tools require gating + confirmation string.
- File permissions: logs created `0600` on Unix; prefer app‑private directories on Windows.

---

## 8. Commit, Branch, and PR Process

### 8.1 Commit Conventions

- Use **Conventional Commits** (`type(scope): subject`) with present‑tense, ≤72‑char subject.
- Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `build`, `ci`, `test`, `chore`.
- Include `BREAKING CHANGE:` footer when applicable.

### 8.2 Branching

- Default branch: `main`.
- Feature branches: `feat/<area>-<slug>`; fixes: `fix/<area>-<slug>`.

### 8.3 Pull Requests

- Checklist (must be satisfied):

  -

- Request at least one reviewer. Squash‑merge unless preserving history is justified.

---

## 9. CI Matrix & Gates

- Platforms: Ubuntu (gcc/clang), macOS (clang), Windows (MSVC).
- Jobs (sequential gates): **format → lint → test → build**.
- Additional nightly jobs: sanitizers (ASan/UBSan/MSan), `clang-tidy` analysis.
- Artifacts: `autogitpull` binary (+ symbols); embed version for `--version`.

---

## 10. Documentation Requirements

- Keep `docs/cli.md` and `examples/` in sync with flags and config keys.
- Provide service wrappers: systemd unit, launchd plist, Windows Task Scheduler XML.
- Troubleshooting: timeouts, auth prompts, diverged branches, high CPU usage, attach failures.

---

## 11. Coding Standards

- Adhere to `.clang-format` and `CPPLINT.cfg`.
- Prefer value semantics; avoid raw owning pointers; use RAII and `std::unique_ptr`/`std::shared_ptr` when needed.
- Use `std::chrono` for time; `std::filesystem` for paths.
- No exceptions for control flow; return `expected<T, E>`‑style or status enums where suitable.
- Threading: use `std::jthread`/`std::thread` with clear ownership; prefer thread pools; avoid data races via `std::mutex`/`std::atomic`; document memory order.
- I/O and subprocess: structured wrappers; timeouts and cancellation tokens required.

---

## 12. Feature Flags & Roadmap Boundaries

- Do not add Git CLI fallback or submodule recursion in MVP paths.
- Submodule flag (`--submodules`) is reserved; do not expose until implemented end‑to‑end.
- MCP server is optional; default off; keep destructive tools gated behind runtime flags and confirmation.

---

## 13. Release Readiness Checklist

-

---

## 14. Make Targets (conventions)

- `make format`, `make lint`, `make tidy` (optional), `make test`, `make` (build), `make dist` (optional packaging), `make clean`.
- Make targets must forward to CMake (`cmake --build`/`ctest`).

---

## 15. Notes for Contributors

- Use temporary directories for tests; never depend on global git config.
- Respect environment variables like `GIT_ASKPASS`, `SSH_AUTH_SOCK` in subprocess execution.
- Prefer structured JSON logs for machine ingestion when adding new events.

---

## 16. Directory Commentary (reference)

- `src/autogitpull.cpp` – CLI entry and main loop.
- `src/git_utils.cpp` – git operations implemented via libgit2.
- `src/tui.cpp` – interactive dashboard.
- `src/config_utils.cpp` – YAML/JSON parsing + precedence logic.
- `src/logger.cpp` – sinks (file/syslog), rotation, redaction.
- `src/resource_utils.cpp` – CPU/mem/thread/net trackers + budgets.
- `src/system_utils.cpp` – OS helpers.
- `src/options.cpp` – flag definitions and validators.
- `src/parse_utils.cpp` – CLI parsing helpers.

---

## 17. Pre‑Commit Hook (optional)

```bash
#!/usr/bin/env bash
set -euo pipefail
make format
make lint
make test
```

Install: `ln -s ../../scripts/pre-commit .git/hooks/pre-commit` (or copy).

---

## 18. License & DCO

- Include Signed‑off‑by lines if project enables DCO. Obey third‑party license obligations for any new dependency.

