CXX = g++
CXXFLAGS = -std=c++20 -pthread -Iinclude $(shell pkg-config --cflags libgit2 2>/dev/null) $(shell pkg-config --cflags yaml-cpp 2>/dev/null) $(shell pkg-config --cflags zlib 2>/dev/null)
UNAME_S := $(shell uname -s)
LIBGIT2_STATIC_AVAILABLE := no
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2) \
              $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp) \
              $(shell pkg-config --libs zlib 2>/dev/null || echo -lz) \
              -framework CoreFoundation -framework CoreServices -framework FSEvents
else
ifeq ($(LIBGIT2_STATIC_AVAILABLE),yes)
    LDFLAGS = $(shell pkg-config --static --libs libgit2) $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp) $(shell pkg-config --libs zlib 2>/dev/null || echo -lz) -static
else
    LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2) $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp) $(shell pkg-config --libs zlib 2>/dev/null || echo -lz)
endif
endif

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
    src/mutant_mode.cpp

# Platform-specific sources
ifeq ($(OS),Windows_NT)
SRC += src/windows_service.cpp src/windows_commands.cpp
else ifeq ($(UNAME_S),Darwin)
SRC += src/macos_daemon.cpp src/linux_commands.cpp
else
SRC += src/linux_daemon.cpp src/linux_commands.cpp
endif

ifeq ($(OS),Windows_NT)
SRC += src/lock_utils_windows.cpp
else
SRC += src/lock_utils_posix.cpp
endif

OBJ = $(SRC:.cpp=.o)
FORMAT_FILES = $(SRC) src/lock_utils_posix.cpp src/lock_utils_windows.cpp include/*.hpp

all: autogitpull

autogitpull: $(OBJ)
	mkdir -p dist
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o dist/autogitpull

test:
	cmake -S . -B build
	cmake --build build
	cd build && ctest --output-on-failure

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -rf dist

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)
	cpplint --linelength=100 $(FORMAT_FILES)

format:
	clang-format -i $(FORMAT_FILES)

deps:
	./scripts/install_deps.sh

.PHONY: all clean lint format deps test
