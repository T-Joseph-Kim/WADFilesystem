CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

SRC = Wad.cpp
HEADER = Wad.h
OBJ = Wad.o
LIB = libWad.a

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $^

$(OBJ): $(SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)

rebuild: clean all
