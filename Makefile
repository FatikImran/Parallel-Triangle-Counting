CXX := g++
CXXFLAGS := -O3 -std=c++17 -Wall -Wextra -march=native -fopenmp
LDFLAGS := -fopenmp

SRC := src/main.cpp src/triangle_counter.cpp src/arch_info.cpp
OBJ := $(SRC:.cpp=.o)
TARGET := triangle_counter

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
