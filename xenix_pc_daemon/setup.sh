#!/bin/bash

# XENIX Server Setup Script
# Run with: sudo ./setup.sh

set -e

echo "====================================="
echo "XENIX Server Installation Script"
echo "====================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script must be run as root (use sudo)"
    exit 1
fi

# Check for required commands
echo "Checking dependencies..."
for cmd in g++ make systemctl; do
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: $cmd is not installed"
        exit 1
    fi
done
echo "✓ All dependencies found"
echo ""

# Compile
echo "Compiling XENIX server..."
make clean 2>/dev/null || true
make
echo "✓ Compilation successful"
echo ""

# Install
echo "Installing XENIX server..."
make install
echo "✓ Installation complete"
echo ""

# Start service
echo "Starting XENIX service..."
systemctl start xenix
sleep 2

# Check status
if systemctl is-active --quiet xenix; then
    echo "✓ Service started successfully"
    echo ""
    echo "====================================="
    echo "XENIX Server is now running!"
    echo "====================================="
    echo ""
    echo "Connect with:"
    echo "  telnet localhost 2323"
    echo ""
    echo "Manage service:"
    echo "  sudo systemctl status xenix   - Check status"
    echo "  sudo systemctl stop xenix     - Stop service"
    echo "  sudo systemctl restart xenix  - Restart service"
    echo "  sudo journalctl -u xenix -f   - View logs"
    echo ""
    echo "To enable on boot:"
    echo "  sudo systemctl enable xenix"
    echo ""
else
    echo "✗ Service failed to start"
    echo ""
    echo "Check logs with:"
    echo "  sudo journalctl -u xenix -n 50"
    exit 1
fi
