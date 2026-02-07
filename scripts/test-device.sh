#!/usr/bin/env bash
# Comprehensive device testing script
# Tests motor, LED, and stream capabilities of connected Kinect

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "════════════════════════════════════════"
echo "Kinect Device Testing Suite"
echo "════════════════════════════════════════"
echo ""

# Check if running with sudo
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  This script requires sudo for USB device access"
    echo "Run with: sudo $0"
    exit 1
fi

# Step 1: USB Enumeration
echo "Step 1: USB Enumeration"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

CAMERA_SERIAL=$(system_profiler SPUSBDataType | grep -A 10 "Xbox NUI Camera" | grep "Serial Number" | awk '{print $3}')
AUDIO_SERIAL=$(system_profiler SPUSBDataType | grep -A 10 "Xbox Kinect Audio" | grep "Serial Number" | awk '{print $3}')

echo "Camera Serial: $CAMERA_SERIAL"
echo "Audio Serial:  $AUDIO_SERIAL"
echo ""

# Step 2: Motor and LED Testing
echo "Step 2: Motor and LED Testing"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$SCRIPT_DIR/list-kinects" ]; then
    "$SCRIPT_DIR/list-kinects"
else
    echo "⚠️  Motor test utility not compiled"
    echo "Run: cd $SCRIPT_DIR && ./build-list-kinects.sh"
fi

echo ""

# Step 3: Integration Tests
echo "Step 3: Integration Tests (Stream Validation)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$REPO_ROOT/build/bin/integration_tests" ]; then
    echo "Running integration tests..."
    "$REPO_ROOT/build/bin/integration_tests"
else
    echo "⚠️  Integration tests not built"
    echo "Run: cd $REPO_ROOT && cmake --build build"
fi

echo ""
echo "════════════════════════════════════════"
echo "Testing Complete"
echo "════════════════════════════════════════"
echo ""
echo "Next steps:"
echo "1. Record results in docs/DEVICE_INVENTORY.md"
echo "2. Check physical label on device for model number"
echo "3. Note any issues or quirks in the inventory"
echo ""
