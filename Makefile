# Convenience Makefile that wraps CMake (authoritative build system)

BUILD_DIR ?= build
CONFIG ?= Release

SRC = \
    src/autogitpull.cpp \
    src/git_utils.cpp \
    src/tui.cpp \
    src/logger.cpp \
    src/resource_utils.cpp \
    src/system_utils.cpp \
    src/time_utils.cpp \
    src/config_utils.cpp \
    src/ignore_utils.cpp \
    src/debug_utils.cpp \
    src/scanner.cpp \
    src/ui_loop.cpp \
    src/file_watch.cpp \
    src/options.cpp \
    src/parse_utils.cpp \
    src/history_utils.cpp \
    src/process_monitor.cpp \
    src/help_text.cpp \
    src/cli_commands.cpp \
    src/mutant_mode.cpp \
    src/lock_utils_posix.cpp \
    src/lock_utils_windows.cpp

FORMAT_FILES = $(SRC) include/*.hpp

.PHONY: all build clean format lint test dist

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG)
	cmake --build $(BUILD_DIR) --config $(CONFIG) -j

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -j

format:
	@if command -v clang-format >/dev/null 2>&1; then \
	  echo "Running clang-format"; \
	  clang-format -i $(FORMAT_FILES); \
	else \
	  echo "clang-format not found; skipping format (install clang-format to enable)"; \
	fi

lint:
	@ret=0; \
	if command -v clang-format >/dev/null 2>&1; then \
	  echo "Checking format"; \
	  clang-format --dry-run --Werror $(FORMAT_FILES) || ret=1; \
	else \
	  echo "clang-format not found; skipping format check"; \
	fi; \
	if command -v cpplint >/dev/null 2>&1; then \
	  echo "Running cpplint"; \
	  cpplint --linelength=120 $(FORMAT_FILES) || ret=1; \
	else \
	  echo "cpplint not found; skipping cpplint (pip install cpplint)"; \
	fi; \
	exit $$ret

clean:
	cmake -E rm -rf $(BUILD_DIR)
