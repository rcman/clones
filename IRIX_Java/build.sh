#!/bin/bash

# IRIX Desktop Build Script (Enhanced)
# Usage: ./build.sh [options]
# Options:
#   --run       Build and run the application
#   --clean     Clean build (remove all .class files first)
#   --jar       Create JAR file after compilation
#   --help      Show this help message

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================"
echo "IRIX Desktop Environment - Build Script"
echo -e "======================================${NC}"
echo ""

# Parse arguments
RUN_AFTER=false
CLEAN_BUILD=false
CREATE_JAR=false

for arg in "$@"; do
    case $arg in
        --run)
            RUN_AFTER=true
            ;;
        --clean)
            CLEAN_BUILD=true
            ;;
        --jar)
            CREATE_JAR=true
            ;;
        --help)
            echo "Usage: ./build.sh [options]"
            echo "Options:"
            echo "  --run       Build and run the application"
            echo "  --clean     Clean build (remove all .class files first)"
            echo "  --jar       Create JAR file after compilation"
            echo "  --help      Show this help message"
            exit 0
            ;;
    esac
done

# Check if Java compiler is available
if ! command -v javac &> /dev/null; then
    echo -e "${RED}ERROR: javac not found. Please install JDK:${NC}"
    echo "  Ubuntu/Debian: sudo apt-get install default-jdk"
    echo "  MacOS: brew install openjdk"
    echo "  Windows: Download from https://www.oracle.com/java/technologies/downloads/"
    exit 1
fi

# Display Java version
echo -e "${YELLOW}Java compiler version:${NC}"
javac -version
echo ""

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning old class files...${NC}"
    rm -f *.class
    rm -f IRIXDesktop.jar
    echo -e "${GREEN}Clean complete.${NC}"
    echo ""
fi

# List of source files in dependency order
SOURCE_FILES=(
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

# Check if all source files exist
echo -e "${YELLOW}Checking source files...${NC}"
MISSING=0
for file in "${SOURCE_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Missing: $file${NC}"
        MISSING=$((MISSING + 1))
    fi
done

if [ $MISSING -gt 0 ]; then
    echo -e "${RED}ERROR: $MISSING source file(s) missing!${NC}"
    exit 1
fi
echo -e "${GREEN}All source files found.${NC}"
echo ""

# Compile all Java files
echo -e "${YELLOW}Compiling Java files...${NC}"
echo "----------------------"

COMPILED=0
FAILED=0

for file in "${SOURCE_FILES[@]}"; do
    echo -n "Compiling $file... "
    if javac -Xlint:none "$file" 2>/dev/null; then
        echo -e "${GREEN}OK${NC}"
        COMPILED=$((COMPILED + 1))
    else
        echo -e "${RED}FAILED${NC}"
        FAILED=$((FAILED + 1))
        # Show actual error
        javac "$file"
    fi
done

echo ""

if [ $FAILED -gt 0 ]; then
    echo -e "${RED}======================================"
    echo "Compilation FAILED!"
    echo "$FAILED file(s) failed to compile."
    echo -e "======================================${NC}"
    exit 1
fi

echo -e "${GREEN}======================================"
echo "Compilation successful!"
echo "$COMPILED file(s) compiled."
echo -e "======================================${NC}"
echo ""

# Create JAR if requested
if [ "$CREATE_JAR" = true ]; then
    echo -e "${YELLOW}Creating JAR file...${NC}"
    
    # Create manifest
    echo "Main-Class: IRIXDesktop" > manifest.txt
    
    # Create JAR
    jar cfm IRIXDesktop.jar manifest.txt *.class
    
    # Cleanup manifest
    rm manifest.txt
    
    echo -e "${GREEN}JAR file created: IRIXDesktop.jar${NC}"
    echo "Run with: java -jar IRIXDesktop.jar"
    echo ""
fi

# Run if requested
if [ "$RUN_AFTER" = true ]; then
    echo -e "${BLUE}Starting IRIX Desktop...${NC}"
    echo ""
    java IRIXDesktop
else
    echo "To run the application:"
    echo "  java IRIXDesktop"
    echo ""
    echo "Or run this script with --run flag:"
    echo "  ./build.sh --run"
fi
