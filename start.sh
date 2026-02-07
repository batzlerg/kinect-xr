#!/bin/bash
# Start Kinect XR Bridge and Web Server
# Usage: ./start.sh
#
# Prerequisites: Run scripts/setup-test-permissions.sh once to configure sudoers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
WEB_DIR="$SCRIPT_DIR/web"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Kinect XR Bridge Startup${NC}"
echo "========================"
echo ""

# Check if bridge binary exists
if [ ! -f "$BUILD_DIR/bin/kinect-bridge" ]; then
    echo "ERROR: Bridge binary not found. Run: cmake --build build"
    exit 1
fi

# Check if web server exists
if [ ! -f "$WEB_DIR/serve.js" ]; then
    echo "ERROR: Web server not found at $WEB_DIR/serve.js"
    exit 1
fi

# Cleanup function
cleanup() {
    echo ""
    echo "Shutting down..."
    kill $BRIDGE_PID 2>/dev/null || true
    kill $WEB_PID 2>/dev/null || true
    exit 0
}
trap cleanup SIGINT SIGTERM

# Start bridge server (requires sudo for USB access)
echo -e "${GREEN}Starting bridge server...${NC}"
sudo "$BUILD_DIR/bin/kinect-bridge" &
BRIDGE_PID=$!

# Wait a moment for bridge to start
sleep 1

# Start web server
echo -e "${GREEN}Starting web server...${NC}"
cd "$WEB_DIR"
bun serve.js &
WEB_PID=$!
cd "$SCRIPT_DIR"

echo ""
echo -e "${GREEN}Both servers running!${NC}"
echo ""
echo "  Dashboard: http://localhost:3000/examples/rgb-depth/"
echo "  WebSocket: ws://localhost:8765/kinect"
echo ""
echo "Press Ctrl+C to stop"
echo ""

# Wait for either process to exit
wait $BRIDGE_PID $WEB_PID
