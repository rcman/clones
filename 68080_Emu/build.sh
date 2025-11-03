#!/bin/bash

echo "=========================================="
echo "  68000 Simulator - Build Script"
echo "=========================================="
echo ""

# Check for SDL2
if ! pkg-config --exists sdl2 sdl2_ttf 2>/dev/null; then
    echo "ERROR: SDL2 or SDL2_ttf not found!"
    echo ""
    echo "Please install SDL2 development libraries:"
    echo "  Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-ttf-dev"
    echo "  Fedora/RHEL:   sudo dnf install SDL2-devel SDL2_ttf-devel"
    echo "  Arch:          sudo pacman -S sdl2 sdl2_ttf"
    echo "  macOS:         brew install sdl2 sdl2_ttf"
    exit 1
fi

echo "SDL2 found: $(pkg-config --modversion sdl2)"
echo "SDL2_ttf found: $(pkg-config --modversion SDL2_ttf)"
echo ""

# Clean previous build
echo "Cleaning previous build..."
make clean > /dev/null 2>&1

# Build
echo "Building simulator..."
if make; then
    echo ""
    echo "=========================================="
    echo "  Build Successful!"
    echo "=========================================="
    echo ""
    echo "To run the simulator:"
    echo "  ./m68k_simulator"
    echo ""
    echo "Or use: make run"
    echo ""
else
    echo ""
    echo "Build failed! Check the error messages above."
    exit 1
fi
