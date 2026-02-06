#!/bin/bash
# Test M3: RGB texture display
# Run this manually with hardware connected
# Expected: Window displays live Kinect RGB video feed

cd "$(dirname "$0")/build"
echo "===================================="
echo "M3 Test: RGB Texture Display"
echo "===================================="
echo ""
echo "Instructions:"
echo "1. Ensure Kinect is connected"
echo "2. Window should display live RGB video"
echo "3. Check console for frame count logs"
echo "4. Close window when validated"
echo ""
echo "Starting viewer (requires sudo for USB)..."
echo ""

sudo ./MetalKinectViewer
