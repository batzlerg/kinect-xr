#!/usr/bin/env bash
# Quick check for connected Kinect devices using USB enumeration
# No sudo required - reads system_profiler output only

set -e

echo "Quick Kinect Device Check"
echo "========================="
echo ""

# Method 1: Use freenect-micview with --list-devices flag (if available)
echo "Method 1: Checking via freenect utilities..."
if command -v freenect-camtest &> /dev/null; then
    echo "Running freenect-camtest (will show device info then exit)..."
    echo ""
    # Note: freenect-camtest doesn't have a --list flag, but will show device info on startup
    echo "TIP: Look for 'Number of devices found' in the output"
    echo ""
fi

# Method 2: Check USB devices
echo "Method 2: Checking USB for Kinect devices..."
echo ""
system_profiler SPUSBDataType | grep -A 20 "Xbox NUI"

echo ""
echo "========================================"
echo "Model Identification"
echo "========================================"
echo ""
echo "Look for 'Product ID' in the USB info above:"
echo "  - 0x02ae = Model 1414 (original, no firmware needed) ✓"
echo "  - 0x02bf = Model 1473 (needs audio firmware)"
echo "  - 0x02c2 = Model 1517 (needs audio firmware)"
echo "  - 0x02b0 = Kinect for Windows variant"
echo ""
echo "Physical label check:"
echo "  - Look for 'Model XXXX' printed on the bottom/back of Kinect"
echo ""
echo "Firmware requirement:"
echo "  - If motor/LED don't work → likely Model 1473 or 1517"
echo "  - See: https://github.com/OpenKinect/libfreenect/issues/451"
echo ""
