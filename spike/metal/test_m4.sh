#!/bin/bash
# Test M4: RGB + Depth side-by-side display
# Run this manually with hardware connected
# Expected: Window displays live Kinect RGB (left) and depth grayscale (right)

cd "$(dirname "$0")/build"
echo "===================================="
echo "M4 Test: Depth Visualization"
echo "===================================="
echo ""
echo "Instructions:"
echo "1. Ensure Kinect is connected"
echo "2. Left half: RGB video"
echo "3. Right half: Depth (grayscale, inverted - closer is brighter)"
echo "4. Check console for frame count logs"
echo "5. Close window when validated"
echo ""
echo "Starting viewer (requires sudo for USB)..."
echo ""

sudo ./MetalKinectViewer
