CPPFLAGS = -Wall -Wextra -std=c++23 -O3

TARGET = ipc-benchmark
all: $(TARGET)

SRC_DIR = ./src

SRC = $(SRC_DIR)/main.cpp
OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean