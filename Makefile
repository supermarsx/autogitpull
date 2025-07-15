CXX = g++
CXXFLAGS = -std=c++17 -pthread $(shell pkg-config --cflags libgit2 2>/dev/null)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDFLAGS = $(shell pkg-config --libs libgit2 2>/dev/null || echo -lgit2)
else
LDFLAGS = $(shell pkg-config --static --libs libgit2 2>/dev/null || echo -lgit2) -static
endif

SRC = autogitpull.cpp git_utils.cpp tui.cpp
OBJ = $(SRC:.cpp=.o)

all: autogitpull

autogitpull: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) autogitpull

.PHONY: all clean
