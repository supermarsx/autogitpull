#!/usr/bin/env bash
set -euo pipefail
# Find all C++ source and header files tracked by git
FILES=$(git ls-files '*.cpp' '*.hpp')

clang-format --dry-run --Werror $FILES
cpplint --linelength=100 $FILES
