#!/usr/bin/env python3
"""Basic script sanity checks for scripts/ files.

This quick verifier is intentionally lightweight and avoids running
any external commands. It checks file presence and some basic
syntactic hints so CI can catch truncated or malformed scripts.
"""
from pathlib import Path
import ast
import sys

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = list((ROOT / 'scripts').glob('*'))

errors = []

def check_sh(p: Path):
    txt = p.read_text(encoding='utf-8', errors='ignore')
    if not txt.startswith('#!'):
        errors.append(f"{p.name}: missing shebang")
    if 'set -e' not in txt and 'set -o errexit' not in txt:
        errors.append(f"{p.name}: missing 'set -e' or equivalent (recommended)")

def check_bat(p: Path):
    txt = p.read_text(encoding='utf-8', errors='ignore')
    if '@echo off' not in txt.lower():
        errors.append(f"{p.name}: missing '@echo off' (recommended)")

def check_py(p: Path):
    try:
        ast.parse(p.read_text(encoding='utf-8'))
    except Exception as e:
        errors.append(f"{p.name}: python parse error: {e}")

def main():
    want = {'build.py', 'gen_docs.py', 'rule_launch_filter.py', 'check-scripts.py'}
    found = {p.name for p in SCRIPTS}
    missing = want - found
    if missing:
        errors.append(f"Missing expected scripts: {', '.join(sorted(missing))}")

    for p in SCRIPTS:
        if p.suffix == '.sh':
            check_sh(p)
        elif p.suffix == '.py':
            check_py(p)
        elif p.suffix == '.bat':
            check_bat(p)

    if errors:
        print("Script sanity checks failed:\n")
        for e in errors:
            print(' -', e)
        return 2

    print("All script checks passed.")
    return 0

if __name__ == '__main__':
    sys.exit(main())
