#!/usr/bin/env bash
# Comprehensive test for all three Kinect devices
# Run this for each device to collect complete data

set -e

if [ "$EUID" -ne 0 ]; then
    echo "⚠️  Requires sudo: sudo $0"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "════════════════════════════════════════════════════════"
echo "Kinect Device Complete Test Suite"
echo "════════════════════════════════════════════════════════"
echo ""

# Step 1: USB Enumeration
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 1: USB Enumeration"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

CAMERA_SERIAL=$(system_profiler SPUSBDataType | grep -A 10 "Xbox NUI Camera" | grep "Serial Number" | awk '{print $3}')
CAMERA_PID=$(system_profiler SPUSBDataType | grep -A 10 "Xbox NUI Camera" | grep "Product ID" | awk '{print $3}')
MOTOR_PID=$(system_profiler SPUSBDataType | grep -A 10 "Xbox NUI Motor" | grep "Product ID" | awk '{print $3}')
AUDIO_SERIAL=$(system_profiler SPUSBDataType | grep -A 10 "Xbox Kinect Audio" | grep "Serial Number" | awk '{print $3}')

echo "Camera Serial:  $CAMERA_SERIAL"
echo "Camera PID:     $CAMERA_PID"
echo "Motor PID:      $MOTOR_PID"
echo "Audio Serial:   $AUDIO_SERIAL"
echo ""

# Step 2: Motor Control Test
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 2: Motor Control Test"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

$SCRIPT_DIR/quick-motor-test.sh

echo ""

# Step 3: Full Device Capabilities
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 3: Full Device Capabilities"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

$SCRIPT_DIR/list-kinects

echo ""

# Step 4: Integration Tests
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 4: Integration Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$REPO_ROOT/build/bin/integration_tests" ]; then
    $REPO_ROOT/build/bin/integration_tests 2>&1 | tail -35
else
    echo "⚠️  Integration tests not built"
    echo "Run: cd $REPO_ROOT && cmake --build build"
fi

echo ""
echo "════════════════════════════════════════════════════════"
echo "Test Complete for Device: $CAMERA_SERIAL"
echo "════════════════════════════════════════════════════════"
echo ""
echo "Summary for DEVICE_INVENTORY.md:"
echo ""
echo "**USB Enumeration:**"
echo "- **Camera Product ID:** $CAMERA_PID"
echo "- **Camera Serial:** $CAMERA_SERIAL"
echo "- **Motor Product ID:** $MOTOR_PID"
echo "- **Audio Serial:** $AUDIO_SERIAL"
echo ""
echo "Next: Copy test results above into docs/DEVICE_INVENTORY.md"
echo ""
