#!/bin/bash

# OS/2 Warp Clone Build Script
# Requires JDK 8 or later

echo "Building OS/2 Warp Clone..."

# Compile
javac OS2Clone.java

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Running OS/2 Clone..."
    java OS2Clone
else
    echo "Build failed. Make sure JDK is installed:"
    echo "  Ubuntu/Debian: sudo apt install default-jdk"
    echo "  macOS: brew install openjdk"
    echo "  Windows: Download from adoptium.net"
fi
