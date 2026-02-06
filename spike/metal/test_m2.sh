#!/bin/bash
cd "$(dirname "$0")/build"
echo "Running Metal Kinect Viewer for 5 seconds..."
sudo ./MetalKinectViewer &
PID=$!
sleep 5
sudo kill $PID 2>/dev/null || true
wait $PID 2>/dev/null || true
echo "Test complete"
