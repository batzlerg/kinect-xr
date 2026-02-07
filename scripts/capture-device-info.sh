#!/usr/bin/env bash
# Extract USB enumeration data for device inventory
# No sudo required - reads system_profiler output only

set -e

echo "Kinect Device Info Capture"
echo "============================"
echo ""

# Extract relevant info from system profiler
USB_INFO=$(system_profiler SPUSBDataType | grep -A 10 "Xbox NUI")

# Parse camera info
CAMERA_PID=$(echo "$USB_INFO" | grep -A 10 "Xbox NUI Camera" | grep "Product ID" | awk '{print $3}')
CAMERA_SERIAL=$(echo "$USB_INFO" | grep -A 10 "Xbox NUI Camera" | grep "Serial Number" | awk '{print $3}')

# Parse motor info
MOTOR_PID=$(echo "$USB_INFO" | grep -A 10 "Xbox NUI Motor" | grep "Product ID" | awk '{print $3}')

# Parse audio info
AUDIO_SERIAL=$(echo "$USB_INFO" | grep -A 10 "Xbox Kinect Audio" | grep "Serial Number" | awk '{print $3}')

# Display formatted output ready for pasting into DEVICE_INVENTORY.md
echo "**USB Enumeration:**"
echo "- **Camera Product ID:** $CAMERA_PID"
echo "- **Camera Serial:** $CAMERA_SERIAL"
echo "- **Motor Product ID:** $MOTOR_PID"
echo "- **Audio Serial:** $AUDIO_SERIAL"
echo ""
echo "**Model Information:**"
echo "- **Model Number:** 1414 (Original Xbox 360 Kinect)"
echo "- **Firmware Required:** No ✓"
echo "- **Physical Label:** _(check device)_"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Copy the above into docs/DEVICE_INVENTORY.md"
echo ""
echo "To test capabilities, run:"
echo "  sudo ./test-device.sh"
echo ""
