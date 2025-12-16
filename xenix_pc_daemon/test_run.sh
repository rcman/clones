#!/bin/bash

# XENIX Server Test Script
# Run the server without installing as a service

set -e

echo "Building XENIX server..."
make clean 2>/dev/null || true
make

echo ""
echo "====================================="
echo "Starting XENIX Server (Test Mode)"
echo "====================================="
echo ""
echo "The server will run in the foreground."
echo "Connect with: telnet localhost 2323"
echo "Press Ctrl+C to stop the server."
echo ""

# Create temporary state directory
mkdir -p /tmp/xenix
export XENIX_STATE_DIR=/tmp/xenix

echo "State directory: /tmp/xenix"
echo ""
echo "Starting server..."
echo ""

./xenix_server
