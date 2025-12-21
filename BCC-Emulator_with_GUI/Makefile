# Makefile for BCC-500 Emulator

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2
NCURSES_FLAGS = -lncurses

# Targets
all: emulator gui-emulator

# Original text-based emulator
emulator: Emulator.cpp
	$(CXX) $(CXXFLAGS) -o emulator Emulator.cpp

# GUI emulator with ncurses
gui-emulator: GuiEmulator.cpp Emulator.cpp Assembler.h
	$(CXX) $(CXXFLAGS) -o gui-emulator GuiEmulator.cpp $(NCURSES_FLAGS)

# Debug build
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -g
debug: all

# Clean build artifacts
clean:
	rm -f emulator gui-emulator *.o

# Run the GUI emulator
run-gui: gui-emulator
	./gui-emulator

# Run the original emulator
run: emulator
	./emulator

# Create example directory if it doesn't exist
setup:
	mkdir -p examples

.PHONY: all clean debug run run-gui setup
