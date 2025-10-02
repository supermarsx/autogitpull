#!/usr/bin/env python3
"""
Cross-platform convenience build script for autogitpull.

Rules:
- Wraps CMake only; no ad-hoc compilation logic.
- Works on Linux/macOS/Windows (Developer PowerShell or regular shell if cmake is in PATH).

Examples:
  python3 scripts/build.py                     # Release, build/ dir
  python3 scripts/build.py --config Debug      # Debug build
  python3 scripts/build.py --build-dir out     # Custom build dir
  python3 scripts/build.py --target install --prefix /usr/local
  python3 scripts/build.py --test              # Build + run tests
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from typing import List


def run(cmd: List[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build autogitpull via CMake")
    parser.add_argument(
        "--config",
        default=os.environ.get("CONFIG", "Release"),
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        help="CMake configuration (default: Release)",
    )
    parser.add_argument(
        "--build-dir",
        default=os.environ.get("BUILD_DIR", "build"),
        help="Out-of-source build directory (default: build)",
    )
    parser.add_argument(
        "--generator",
        "-G",
        dest="generator",
        default=os.environ.get("CMAKE_GENERATOR"),
        help="CMake generator to use (optional)",
    )
    parser.add_argument(
        "--parallel",
        "-j",
        dest="parallel",
        type=int,
        default=int(os.environ.get("PARALLEL", "0") or 0),
        help="Parallel jobs for build (0 lets CMake choose)",
    )
    parser.add_argument(
        "--target",
        default=None,
        help="Specific build target (e.g., autogitpull, autogitpull_tests)",
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Run tests after building (ctest)",
    )
    parser.add_argument(
        "--install",
        action="store_true",
        help="Run 'cmake --install' after build",
    )
    parser.add_argument(
        "--prefix",
        default=os.environ.get("PREFIX"),
        help="Install prefix for 'cmake --install' (optional)",
    )
    args = parser.parse_args()

    # Verify cmake presence early
    if shutil.which("cmake") is None:
        print("error: cmake not found in PATH", file=sys.stderr)
        return 2

    build_dir = os.path.abspath(args.build_dir)
    os.makedirs(build_dir, exist_ok=True)

    # Configure
    cfg_cmd = ["cmake", "-S", ".", "-B", build_dir, f"-DCMAKE_BUILD_TYPE={args.config}"]
    if args.generator:
        cfg_cmd.extend(["-G", args.generator])
    run(cfg_cmd)

    # Build
    build_cmd = ["cmake", "--build", build_dir, "--config", args.config]
    if args.parallel and args.parallel > 0:
        build_cmd.extend(["-j", str(args.parallel)])
    if args.target:
        build_cmd.extend(["--target", args.target])
    run(build_cmd)

    # Optional: install
    if args.install:
        install_cmd = ["cmake", "--install", build_dir]
        if args.prefix:
            install_cmd.extend(["--prefix", args.prefix])
        run(install_cmd)

    # Optional: test
    if args.test:
        ctest = shutil.which("ctest") or "ctest"
        test_cmd = [ctest, "--test-dir", build_dir, "--output-on-failure", "-C", args.config]
        run(test_cmd)

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode if e.returncode is not None else 1)

