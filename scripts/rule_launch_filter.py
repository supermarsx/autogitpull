#!/usr/bin/env python3
"""
Rule launch filter for CMake link/compile commands.

Strips problematic cross-arch flags (notably -mabi=lp64) that can leak from
CI environments into host x86_64 builds and cause GCC failures on Ubuntu.

Usage: set as CMake RULE_LAUNCH_LINK (or RULE_LAUNCH_COMPILE) for targets.
This script receives the real tool invocation as argv and execs it after
filtering.
"""
from __future__ import annotations

import os
import sys
from typing import List


REMOVALS_PREFIX = (
    "-mabi=",
)


def _filter_args(argv: List[str]) -> List[str]:
    out: List[str] = []
    for a in argv:
        if any(a.startswith(p) for p in REMOVALS_PREFIX):
            continue
        # Also scrub if embedded inside -Wl,<...>
        if a.startswith("-Wl,") and any("-mabi=" in chunk for chunk in a.split(",")):
            # drop the offending chunks and keep the rest
            kept = [c for c in a.split(",") if "-mabi=" not in c]
            if len(kept) <= 1:
                # nothing useful remains -> skip entirely
                continue
            a = ",".join(kept)
        out.append(a)
    return out


def main(argv: List[str]) -> int:
    if len(argv) < 2:
        print("usage: rule_launch_filter.py <cmd> [args...]", file=sys.stderr)
        return 2
    filtered = _filter_args(argv[1:])
    if not filtered:
        print("error: nothing to execute after filtering", file=sys.stderr)
        return 2
    os.execvp(filtered[0], filtered)
    return 127


if __name__ == "__main__":
    sys.exit(main(sys.argv))

