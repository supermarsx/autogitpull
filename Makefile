CXX = g++
CXXFLAGS = -std=c++17 -pthread $(shell pkg-config --cflags libgit2 2>/dev/null)
UNAME_S := $(shell uname -s)
LIBGIT2_STATIC_AVAILABLE := $(shell pkg-config --static --libs libgit2 >/dev/null 2>&1 && echo yes)
ifeq ($(UNAME_S),Darwin)
LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2)
else
ifeq ($(LIBGIT2_STATIC_AVAILABLE),yes)
LDFLAGS = $(shell pkg-config --static --libs libgit2) -static
else
LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2)
endif
endif

SRC = autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils.cpp time_utils.cpp
OBJ = $(SRC:.cpp=.o)
FORMAT_FILES = $(SRC) *.hpp

all: autogitpull

autogitpull: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) autogitpull

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)
	cpplint --linelength=100 $(FORMAT_FILES)
	npx prettier --check "**/*.{md,json}"

deps:
	./install_deps.sh

test:
	cmake -S . -B build
	cmake --build build
	cd build && ctest --output-on-failure

.PHONY: all clean lint deps test
