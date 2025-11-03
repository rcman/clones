#!/bin/bash

# IRIX Desktop Build Script
# This script compiles all Java files and runs the application

echo "======================================"
echo "IRIX Desktop Environment - Build Script"
echo "======================================"
echo ""

# Check if Java compiler is available
if ! command -v javac &> /dev/null
then
    echo "ERROR: javac not found. Please install JDK:"
    echo "  Ubuntu/Debian: sudo apt-get install default-jdk"
    echo "  MacOS: brew install openjdk"
    echo "  Windows: Download from https://www.oracle.com/java/technologies/downloads/"
    exit 1
fi

# Display Java version
echo "Java compiler version:"
javac -version
echo ""

# Clean old class files
echo "Cleaning old class files..."
rm -f *.class
echo ""

# Compile all Java files
echo "Compiling Java files..."
echo "----------------------"

# List of files in dependency order
files=(
    "IRIXWindow.java"
    "Calculator.java"
    "DrawingApp.java"
    "FileSystemBrowser.java"
    "MediaPlayer.java"
    "TerminalEmulator.java"
    "TextEditor.java"
    "WebBrowser.java"
    "WorkspaceManager.java"
    "StartMenu.java"
    "IRIXDesktop.java"
)

# Compile each file
for file in "${files[@]}"; do
    echo "Compiling $file..."
    javac "$file"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to compile $file"
        exit 1
    fi
done

echo ""
echo "======================================"
echo "Compilation successful!"
echo "======================================"
echo ""
echo "To run the application, use:"
echo "  java IRIXDesktop"
echo ""
echo "Or run this script with --run flag:"
echo "  ./build.sh --run"
echo ""

# Run if --run flag is provided
if [ "$1" == "--run" ]; then
    echo "Starting IRIX Desktop..."
    echo ""
    java IRIXDesktop
fi
