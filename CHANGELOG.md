# Changelog

All notable changes to this project are documented here. This project follows Conventional Commits for commit messages.

## [0.1.0] - 2025-10-02

Initial public preview release with a full cross‑platform build, extensive tests, and libgit2‑based operations.

Highlights
- Fast, safe repository scanning with opt‑in recursion and ignore support
- libgit2‑backed pull/clone/fetch with timeouts and resource limits
- Deterministic CLI with structured logging, rotation, and optional JSON
- Windows/macOS/Linux support, with CI matrix and tests

Features
- feat(pull): allow targeting refs via CLI and per‑repo overrides (88e6bb8)

Performance
- perf(ignore): fast‑path non‑glob patterns for ignore matching (00db92e)

Fixes
- fix(build/windows): ensure bundled zlib usage for libgit2; eliminate stale AdditionalDependencies (1be7d5e, e3d6c23, c0d0d58, 05a2460, 77e8d07, 243c112, 6dafb31, 838f366, 94d6d9f)
- fix(build): guard/avoid duplicate zlib targets; enforce build order (3976cd4, d18ef8a, 30dacaf, c7d0515, 302ea19, add2a6f)
- fix(macos): avoid system pip; fix zlib multi‑arch linking and Security framework usage (7c2adb3, 147fc07, cab6d51)
- fix(ci): Windows and recent‑release scripting portability; coverage reporting improvements (540b083, 2113b9c)
- fix(scripts): install script missing array initialization (9ad6c42)

CI
- ci(windows/arm64): add Windows ARM64 builds and runners (10f9a63, 972ce58)
- ci(test): add Linux arm64 and macOS arm64 jobs; stabilize timing (0444e25, 7dde7d2)
- ci(rolling): badges, compact invocation, portable date handling (f67a3ff, 81c9b9b, 1561e4b, faf8e15)
- ci: normalize matrix order and keep windows‑11‑arm entries (85f3da8)

Build
- build(ubuntu): prefer PIC zlib and guard ZLIB linkage paths (f6d8c78, 33c7b9e, 104bd19, 5ef6d81, 6e03d69)
- build: CMake improvements around bundled zlib and dependency probing (c0d0d58, 302ea19, 1ae5137)

Tests
- test: deflake local clone/syslog checks; discover Catch cases individually (f9843ff, 05c0936)
- test(macos): stabilize instance detection suite (00c3c9b)
- test(resource): stabilize rate limit timing (7dde7d2)

Docs
- docs: add rolling‑release CI badge to README (f67a3ff)
- readme/gitignore updates (e168978)

Chore/Formatting
- chore(format): auto‑apply clang‑format (da709ed, 1ae5137)

Notes
- Version embedded via `include/version.hpp` and surfaced by `--version`.
- Builds are produced via CMake presets across Windows/macOS/Linux.

[0.1.0]: https://example.com/autogitpull/releases/0.1.0

