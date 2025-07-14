CXX = g++
CXXFLAGS = -std=c++17 -pthread
LDFLAGS = -lgit2

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
