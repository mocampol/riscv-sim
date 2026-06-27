CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET = riscv-sim
SRCS = src/main.cpp src/cpu.cpp src/memory.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
