CXX = g++
CXXFLAGS = -std=c++17 -pthread $(shell pkg-config --cflags libgit2 2>/dev/null)
LDFLAGS = $(shell pkg-config --static --libs libgit2 2>/dev/null || echo -lgit2) -static

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
