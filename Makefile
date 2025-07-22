CXX = g++
HAS_B := $(shell command -v b >/dev/null 2>&1 && echo yes || echo no)
CXXFLAGS = -std=c++20 -pthread -Iinclude $(shell pkg-config --cflags libgit2 2>/dev/null) $(shell pkg-config --cflags yaml-cpp 2>/dev/null)
UNAME_S := $(shell uname -s)
LIBGIT2_STATIC_AVAILABLE := $(shell pkg-config --static --libs libgit2 >/dev/null 2>&1 && echo yes)
ifeq ($(UNAME_S),Darwin)
LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2) $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp)
else
ifeq ($(LIBGIT2_STATIC_AVAILABLE),yes)
LDFLAGS = $(shell pkg-config --static --libs libgit2) $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp) -static
else
LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2) $(shell pkg-config --libs yaml-cpp 2>/dev/null || echo -lyaml-cpp)
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
    src/debug_utils.cpp \
    src/options.cpp \
    src/parse_utils.cpp \
    src/lock_utils.cpp \
    src/linux_daemon.cpp \
    src/windows_service.cpp
OBJ = $(SRC:.cpp=.o)
FORMAT_FILES = $(SRC) include/*.hpp

ifeq ($(HAS_B),yes)
all:
	b install config.install.root=dist

test:
	b test
else
all: autogitpull

autogitpull: $(OBJ)
	mkdir -p dist
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o dist/autogitpull

test:
	cmake -S . -B build
	cmake --build build
	cd build && ctest --output-on-failure
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -rf dist

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)
	cpplint --linelength=100 $(FORMAT_FILES)

deps:
	./scripts/install_deps.sh

.PHONY: all clean lint deps test
