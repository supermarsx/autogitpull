# Convenience Makefile that wraps CMake (authoritative build system)

BUILD_DIR ?= build
CONFIG ?= Release

PARALLEL ?= 8
TEST_PARALLEL ?= 1

PROJECT_ROOT := $(CURDIR)
FORMAT_SCRIPT := scripts/clang_format.cmake
CPPLINT_SCRIPT := scripts/run_cpplint.cmake

.PHONY: all build clean format lint test dist

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG)
	cmake --build $(BUILD_DIR) --config $(CONFIG) -j $(PARALLEL)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -j $(TEST_PARALLEL) -C $(CONFIG) -R autogitpull_tests

.PHONY: test-all
test-all: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -j $(TEST_PARALLEL) -C $(CONFIG)

format:
	@cmake -DPROJECT_ROOT="$(PROJECT_ROOT)" -DACTION=fix -P $(FORMAT_SCRIPT)

lint:
	@cmake -DPROJECT_ROOT="$(PROJECT_ROOT)" -DACTION=check -P $(FORMAT_SCRIPT)
	@cmake -DPROJECT_ROOT="$(PROJECT_ROOT)" -P $(CPPLINT_SCRIPT)

clean:
	cmake -E rm -rf $(BUILD_DIR)
