# Build options begin
#USE_ASAN = true
# Build options end

NULL =
CXXFLAGS_ASAN =
CXXFLAGS = -Wall -Wextra -std=c++23 -O3 -fno-tree-vectorize$(CXXFLAGS_ASAN)

ifdef USE_ASAN
	CXXFLAGS_ASAN = $(NULL) -g -fsanitize=address -fno-omit-frame-pointer
endif

TARGET = ipc-benchmark
all: $(TARGET)

SRC_DIR = ./src

SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/data.cpp
OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -lWinRing0x64 -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean