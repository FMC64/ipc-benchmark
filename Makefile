# Build options begin
#USE_ASAN = true
# Build options end

NULL =
CXXFLAGS_ASAN =
CXXFLAGS = -Wall -Wextra -std=c++23 -O3$(CXXFLAGS_ASAN)

ifdef USE_ASAN
	CXXFLAGS_ASAN = $(NULL) -fsanitize=address -fno-omit-frame-pointer
endif

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